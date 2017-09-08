// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/flags/GlobalFlagEnvironment.h"

#include <algorithm>
#include <memory>
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExceptionCode.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/workers/WorkerGlobalScope.h"
#include "core/workers/WorkerThread.h"
#include "modules/flags/Flag.h"
#include "modules/flags/FlagOptions.h"
#include "platform/bindings/Microtask.h"
#include "platform/bindings/ScriptState.h"
#include "public/platform/InterfaceProvider.h"
#include "public/platform/Platform.h"

namespace blink {

namespace {

// Per-context supplement that holds the service interface provider.
class FlagsServiceBridge final
    : public GarbageCollectedFinalized<FlagsServiceBridge>,
      public Supplement<ExecutionContext> {
  USING_GARBAGE_COLLECTED_MIXIN(FlagsServiceBridge);
  WTF_MAKE_NONCOPYABLE(FlagsServiceBridge);

 public:
  static FlagsServiceBridge* From(ExecutionContext* context) {
    DCHECK(context);
    DCHECK(context->IsContextThread());
    FlagsServiceBridge* bridge = static_cast<FlagsServiceBridge*>(
        Supplement<ExecutionContext>::From(context, SupplementName()));
    if (!bridge) {
      bridge = new FlagsServiceBridge(*context);
      Supplement<ExecutionContext>::ProvideTo(*context, SupplementName(),
                                              bridge);
    }
    return bridge;
  }

  static const char* SupplementName() { return "FlagsServiceBridge"; }

  ~FlagsServiceBridge() {}

  mojom::blink::FlagsService* GetService() {
    if (!service_.get()) {
      ExecutionContext* context = GetSupplementable();
      DCHECK(context);
      DCHECK(context->IsContextThread());
      service_manager::InterfaceProvider* provider;
      if (context->IsDocument()) {
        // Frame-scoped provider is preferred; the requested can be trusted.
        LocalFrame* frame = ToDocument(context)->GetFrame();
        // TODO(jsbell): When is there no frame?
        if (!frame)
          return nullptr;
        provider = &frame->GetInterfaceProvider();
      } else if (context->IsWorkerGlobalScope()) {
        // Workers do not yet have a dedicated provider; this is an alias
        // for Platform::GetInterfaceProvider and uses the registration
        // in RenderProcessHostImpl.
        // TODO(jsbell): Replace with a frame-scoped provider for the worker.
        provider =
            &ToWorkerGlobalScope(context)->GetThread()->GetInterfaceProvider();
      } else {
        NOTREACHED();
        return nullptr;
      }
      provider->GetInterface(mojo::MakeRequest(&service_));
    }

    return service_.get();
  }

 private:
  explicit FlagsServiceBridge(ExecutionContext& context)
      : Supplement<ExecutionContext>(context) {}

  mojom::blink::FlagsServicePtr service_;
};

class FlagRequest final {
  WTF_MAKE_NONCOPYABLE(FlagRequest);

 public:
  FlagRequest(ScriptPromiseResolver* resolver,
              RefPtr<SecurityOrigin> origin,
              const Vector<String>& scope,
              mojom::blink::FlagMode mode,
              FlagsServiceBridge* bridge)
      : resolver_(resolver),
        scope_(scope),
        mode_(mode),
        origin_(std::move(origin)),
        bridge_(bridge) {}

  ~FlagRequest() {}

  void Call(int64_t flag_id) {
    if (!resolver_->GetExecutionContext() ||
        resolver_->GetExecutionContext()->IsContextDestroyed()) {
      if (flag_id != mojom::blink::FlagsService::kFlagTimeout) {
        mojom::blink::FlagsService* service = bridge_->GetService();
        if (service)
          service->ReleaseFlag(origin_, flag_id);
      }
      return;
    }

    if (flag_id == mojom::blink::FlagsService::kFlagTimeout) {
      resolver_->Reject(DOMException::Create(
          kTimeoutError,
          "The flag could not be acquired before the specified timeout."));
      resolver_.Clear();
      return;
    }

    ScriptState* script_state = resolver_->GetScriptState();
    ScriptState::Scope scope(script_state);

    Flag* flag = Flag::Create(
        resolver_->GetScriptState(), scope_, mode_,
        WTF::Bind(&FlagRequest::ReleaseTask, bridge_, origin_, flag_id));
    resolver_->Resolve(flag);

    // This implements the subtle auto-release shenanigans outlined in the gist.
    // https://gist.github.com/inexorabletash/a53c6add9fbc8b9b1191

    ScriptPromiseResolver* waiting_resolver =
        ScriptPromiseResolver::Create(script_state);
    ScriptPromise waiting = waiting_resolver->Promise();

    NonThrowableExceptionState does_not_throw;
    flag->waitUntil(script_state, waiting, does_not_throw);

    // TODO(jsbell): Can this be done without the lambda?
    Microtask::EnqueueMicrotask(
        WTF::Bind([](ScriptPromiseResolver* resolver) { resolver->Resolve(); },
                  WTF::Passed(std::move(waiting_resolver))));

    resolver_.Clear();
  }

 private:
  // Wrapped up in a closure given to the Flag instance so it can self-release.
  static void ReleaseTask(FlagsServiceBridge* bridge,
                          RefPtr<SecurityOrigin> origin,
                          int64_t flag_id) {
    mojom::blink::FlagsService* service = bridge->GetService();
    if (service)
      service->ReleaseFlag(origin, flag_id);
  }

  // Resolves/rejects the promise with a Flag when the flag is acquired.
  Persistent<ScriptPromiseResolver> resolver_;

  // Held to stamp the Flag object's |scope| property.
  Vector<String> scope_;

  // Held to stamp the Flag object's |mode| property.
  mojom::blink::FlagMode mode_;

  // Used to release the flag by id - either immediately (if the flag arrives
  // after the context is being torn down) or via closure passed to the Flag
  // itself.
  RefPtr<SecurityOrigin> origin_;
  Persistent<FlagsServiceBridge> bridge_;
};

// Implementation of |requestFlag()| used in both window and worker contexts.
ScriptPromise RequestFlagCommon(ScriptState* script_state,
                                const StringOrStringSequence& scope_union,
                                const String& mode_string,
                                const FlagOptions& options,
                                ExceptionState& exception_state) {
  // Compute the scope (set of named resources).
  HashSet<String> set;
  if (scope_union.isString()) {
    set.insert(scope_union.getAsString());
  } else if (scope_union.isStringSequence()) {
    for (const auto& name : scope_union.getAsStringSequence())
      set.insert(name);
  } else {
    NOTREACHED();
    return ScriptPromise();
  }

  if (set.IsEmpty()) {
    exception_state.ThrowTypeError("Scope must not be empty");
    return ScriptPromise();
  }

  Vector<String> scope;
  for (const auto& name : set)
    scope.push_back(name);
  std::sort(scope.begin(), scope.end(), WTF::CodePointCompareLessThan);

  mojom::blink::FlagMode mode = Flag::StringToMode(mode_string);

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  RefPtr<SecurityOrigin> origin =
      ExecutionContext::From(script_state)->GetSecurityOrigin();

  FlagsServiceBridge* bridge =
      FlagsServiceBridge::From(ExecutionContext::From(script_state));

  bridge->GetService()->RequestFlag(
      origin, scope, mode,
      ConvertToBaseCallback(WTF::Bind(
          &FlagRequest::Call, WTF::Passed(WTF::MakeUnique<FlagRequest>(
                                  resolver, origin, scope, mode, bridge)))));

  return promise;
}

}  // namespace

ScriptPromise GlobalFlagEnvironment::requestFlag(
    ScriptState* script_state,
    DOMWindow&,
    const StringOrStringSequence& scope,
    const String& mode,
    const FlagOptions& options,
    ExceptionState& exception_state) {
  return RequestFlagCommon(script_state, scope, mode, options, exception_state);
}

ScriptPromise GlobalFlagEnvironment::requestFlag(
    ScriptState* script_state,
    WorkerGlobalScope&,
    const StringOrStringSequence& scope,
    const String& mode,
    const FlagOptions& options,
    ExceptionState& exception_state) {
  return RequestFlagCommon(script_state, scope, mode, options, exception_state);
}

}  // namespace blink
