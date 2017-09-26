// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/locks/GlobalLockEnvironment.h"

#include <algorithm>
#include <memory>
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/DOMException.h"
#include "core/dom/ExceptionCode.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/workers/WorkerGlobalScope.h"
#include "core/workers/WorkerThread.h"
#include "modules/locks/Lock.h"
#include "modules/locks/LockOptions.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "platform/bindings/Microtask.h"
#include "platform/bindings/ScriptState.h"
#include "public/platform/InterfaceProvider.h"
#include "public/platform/Platform.h"

namespace blink {

namespace {

class LockRequestImpl;

// Per-context supplement that holds the service interface provider.
class LocksServiceBridge final
    : public GarbageCollectedFinalized<LocksServiceBridge>,
      public Supplement<ExecutionContext>,
      public ContextLifecycleObserver {
  USING_GARBAGE_COLLECTED_MIXIN(LocksServiceBridge);
  WTF_MAKE_NONCOPYABLE(LocksServiceBridge);

 public:
  static LocksServiceBridge* From(ExecutionContext* context) {
    DCHECK(context);
    DCHECK(context->IsContextThread());
    LocksServiceBridge* bridge = static_cast<LocksServiceBridge*>(
        Supplement<ExecutionContext>::From(context, SupplementName()));
    if (!bridge) {
      bridge = new LocksServiceBridge(*context);
      Supplement<ExecutionContext>::ProvideTo(*context, SupplementName(),
                                              bridge);
    }
    return bridge;
  }

  static const char* SupplementName() { return "LocksServiceBridge"; }

  ~LocksServiceBridge() = default;

  DEFINE_INLINE_VIRTUAL_TRACE() {
    Supplement<ExecutionContext>::Trace(visitor);
    ContextLifecycleObserver::Trace(visitor);
  }

  // Terminate all outstanding requests when the context is destroyed, since
  // this can unblock requests by other contexts.
  void ContextDestroyed(ExecutionContext*) override {
    pending_requests_.clear();
  }

  mojom::blink::LocksService* GetService() {
    ExecutionContext* context = GetExecutionContext();

    // After it is detached.
    if (!context)
      return nullptr;

    if (!service_.get()) {
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

  void AddPendingRequest(std::unique_ptr<LockRequestImpl> request) {
    pending_requests_.insert(std::move(request));
  }

  std::unique_ptr<LockRequestImpl> RemovePendingRequest(
      LockRequestImpl* request) {
    return pending_requests_.Take(request);
  }

 private:
  explicit LocksServiceBridge(ExecutionContext& context)
      : Supplement<ExecutionContext>(context),
        ContextLifecycleObserver(&context) {}

  WTF::HashSet<std::unique_ptr<LockRequestImpl>> pending_requests_;

  mojom::blink::LocksServicePtr service_;
};

class LockRequestImpl final : public mojom::blink::LockRequest {
  WTF_MAKE_NONCOPYABLE(LockRequestImpl);

 public:
  LockRequestImpl(ScriptPromiseResolver* resolver,
                  const Vector<String>& scope,
                  mojom::blink::LockMode mode,
                  mojom::blink::LockRequestRequest request,
                  LocksServiceBridge* bridge)
      : resolver_(resolver),
        scope_(scope),
        mode_(mode),
        binding_(this, std::move(request)),
        bridge_(bridge) {}

  ~LockRequestImpl() = default;

  // TODO: This would be called when the AbortController signals.
  void Abort() {
    // |this| will be destroyed when this method returns.
    bridge_->RemovePendingRequest(this);
  }

  void Granted(mojom::blink::LockHandlePtr handle) {
    DCHECK(binding_.is_bound());

    // |this| will be destroyed when this method returns.
    std::unique_ptr<LockRequestImpl> self = bridge_->RemovePendingRequest(this);

    if (!resolver_->GetExecutionContext() ||
        resolver_->GetExecutionContext()->IsContextDestroyed()) {
      // If a handle was returned, it will be automatically be released.
      return;
    }

    // TODO: Can we DCHECK this never happens?
    if (!handle.is_bound()) {
      resolver_->Reject(
          DOMException::Create(kAbortError, "The lock could not be acquired."));
      return;
    }

    ScriptState* script_state = resolver_->GetScriptState();
    ScriptState::Scope scope(script_state);

    Lock* lock = Lock::Create(resolver_->GetScriptState(), scope_, mode_,
                              std::move(handle));
    resolver_->Resolve(lock);

    // This implements the subtle auto-release shenanigans outlined in the gist.
    // https://github.com/inexorabletash/origin-locks

    ScriptPromiseResolver* waiting_resolver =
        ScriptPromiseResolver::Create(script_state);
    ScriptPromise waiting = waiting_resolver->Promise();

    NonThrowableExceptionState does_not_throw;
    lock->waitUntil(script_state, waiting, does_not_throw);

    // Lambda is used because binding to overloaded methods is confusing.
    Microtask::EnqueueMicrotask(
        WTF::Bind([](ScriptPromiseResolver* resolver) { resolver->Resolve(); },
                  WTF::Passed(std::move(waiting_resolver))));
  }

 private:
  // Resolves/rejects the promise with a Lock when the lock is acquired.
  Persistent<ScriptPromiseResolver> resolver_;

  // Held to stamp the Lock object's |scope| property.
  Vector<String> scope_;

  // Held to stamp the Lock object's |mode| property.
  mojom::blink::LockMode mode_;

  mojo::Binding<mojom::blink::LockRequest> binding_;

  // The |bridge_| keeps |this| alive until a response comes in and this is
  // registered. If the context is destroyed then |bridge_| will dispose of
  // |this| which terminates the request on the service side.
  WeakPersistent<LocksServiceBridge> bridge_;
};

// Implementation of |requestLock()| used in both window and worker contexts.
ScriptPromise RequestLockCommon(ScriptState* script_state,
                                const StringOrStringSequence& scope_union,
                                const String& mode_string,
                                const LockOptions& options,
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

  mojom::blink::LockMode mode = Lock::StringToMode(mode_string);

  LocksServiceBridge* bridge =
      LocksServiceBridge::From(ExecutionContext::From(script_state));

  mojom::blink::LocksService* service = bridge->GetService();
  if (!service) {
    exception_state.ThrowTypeError("Service not available");
    return ScriptPromise();
  }

  ScriptPromiseResolver* resolver = ScriptPromiseResolver::Create(script_state);
  ScriptPromise promise = resolver->Promise();

  mojom::blink::LockRequestPtr request_ptr;
  std::unique_ptr<LockRequestImpl> request = WTF::MakeUnique<LockRequestImpl>(
      resolver, scope, mode, mojo::MakeRequest(&request_ptr), bridge);

  service->RequestLock(
      ExecutionContext::From(script_state)->GetSecurityOrigin(), scope, mode,
      std::move(request_ptr));

  bridge->AddPendingRequest(std::move(request));

  return promise;
}

}  // namespace

ScriptPromise GlobalLockEnvironment::requestLock(
    ScriptState* script_state,
    DOMWindow&,
    const StringOrStringSequence& scope,
    const String& mode,
    const LockOptions& options,
    ExceptionState& exception_state) {
  return RequestLockCommon(script_state, scope, mode, options, exception_state);
}

ScriptPromise GlobalLockEnvironment::requestLock(
    ScriptState* script_state,
    WorkerGlobalScope&,
    const StringOrStringSequence& scope,
    const String& mode,
    const LockOptions& options,
    ExceptionState& exception_state) {
  return RequestLockCommon(script_state, scope, mode, options, exception_state);
}

}  // namespace blink
