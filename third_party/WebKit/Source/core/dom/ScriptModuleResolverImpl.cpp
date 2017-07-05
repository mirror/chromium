// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ScriptModuleResolverImpl.h"

#include "bindings/core/v8/ScriptModule.h"
#include "core/dom/Modulator.h"
#include "core/dom/ModuleScript.h"
#include "core/loader/modulescript/ModuleScriptFetchRequest.h"

namespace blink {

void ScriptModuleResolverImpl::RegisterModuleScript(
    ModuleScript* module_script) {
  DCHECK(module_script);
  if (module_script->Record().IsNull())
    return;

  DVLOG(1) << "ScriptModuleResolverImpl::RegisterModuleScript(url="
           << module_script->BaseURL().GetString()
           << ", hash=" << ScriptModuleHash::GetHash(module_script->Record())
           << ")";

  auto result =
      record_to_module_script_map_.Set(module_script->Record(), module_script);
  DCHECK(result.is_new_entry);
}

void ScriptModuleResolverImpl::UnregisterModuleScript(
    ModuleScript* module_script) {
  DCHECK(module_script);
  if (module_script->Record().IsNull())
    return;

  DVLOG(1) << "ScriptModuleResolverImpl::UnregisterModuleScript(url="
           << module_script->BaseURL().GetString()
           << ", hash=" << ScriptModuleHash::GetHash(module_script->Record())
           << ")";

  record_to_module_script_map_.erase(module_script->Record());
}

ScriptModule ScriptModuleResolverImpl::Resolve(
    const String& specifier,
    const ScriptModule& referrer,
    ExceptionState& exception_state) {
  // https://html.spec.whatwg.org/commit-snapshots/f8e75a974ed9185e5b462bc5b2dfb32034bd1145#hostresolveimportedmodule(referencingmodule,-specifier)
  DVLOG(1) << "ScriptModuleResolverImpl::resolve(specifier=\"" << specifier
           << ", referrer.hash=" << ScriptModuleHash::GetHash(referrer) << ")";

  // Step 1. Let referencing module script be referencingModule.[[HostDefined]].
  const auto it = record_to_module_script_map_.find(referrer);
  CHECK_NE(it, record_to_module_script_map_.end())
      << "Failed to find referrer ModuleScript corresponding to the record";
  ModuleScript* referrer_module = it->value;

  // Step 2. Let moduleMap be referencing module script's settings object's
  // module map. Note: Blink finds out "module script's settings object"
  // (Modulator) from context where HostResolveImportedModule was called.

  // Step 3. Let url be the result of resolving a module specifier given
  // referencing module script and specifier. If the result is failure, then
  // throw a TypeError exception and abort these steps.
  KURL url =
      Modulator::ResolveModuleSpecifier(specifier, referrer_module->BaseURL());
  if (!url.IsValid()) {
    exception_state.ThrowTypeError("Failed to resolve module specifier '" +
                                   specifier + "'");
    return ScriptModule();
  }

  // Step 4. Let resolved module script be moduleMap[url]. If no such entry
  // exists, or if resolved module script is null or "fetching", then throw a
  // TypeError exception and abort these steps.
  ModuleScript* module_script = modulator_->GetFetchedModuleScript(url);
  if (!module_script) {
    exception_state.ThrowTypeError(
        "Failed to load module script for module specifier '" + specifier +
        "'");
    return ScriptModule();
  }

  // Step 5. If resolved module script's instantiation state is "errored", then
  // throw resolved module script's instantiation error.
  // TODO(kouhei): Update spec references.
  if (module_script->IsErrored()) {
    ScriptValue error = modulator_->GetError(module_script);
    exception_state.RethrowV8Exception(error.V8Value());
    return ScriptModule();
  }

  // Step 6. Assert: resolved module script's module record is not null.
  ScriptModule record = module_script->Record();
  CHECK(!record.IsNull());

  // Step 7. Return resolved module script's module record.
  return record;
}

class ScriptModuleResolverImpl::DynamicResolver : public ModuleTreeClient {
 public:
  static DynamicResolver* Create(Modulator* modulator,
                                 ScriptPromiseResolver* promise_resolver) {
    return new DynamicResolver(modulator, promise_resolver);
  }

  DECLARE_TRACE();

 private:
  DynamicResolver(Modulator* modulator, ScriptPromiseResolver* promise_resolver)
      : modulator_(modulator), promise_resolver_(promise_resolver) {}

  // Implements ModuleTreeClient:
  void NotifyModuleLoadFinished(ModuleScript*) final;

  Member<Modulator> modulator_;
  Member<ScriptPromiseResolver> promise_resolver_;
};

ScriptModuleResolverImpl::DynamicResolver::NotifyModuleLoadFinished(
    ModuleScript* module_script) {
  ScriptState* script_state = modulator_->GetScriptState();
  ScriptState::Scope scope(script_state);
  v8::Isolate* isolate = script_state->GetIsolate();

  // https://github.com/tc39/proposal-dynamic-import/blob/master/HTML%20Integration.md#hostimportmoduledynamicallyreferencingscriptormodule-specifier-promisecapability
  // Step 12. "Let module script be the asynchronous completion value of
  // fetching a module script tree." This is |module_script| here.

  // Step 8. "If fetching a module script tree asynchronously completes with
  // null, then perform the following steps:" [spec text]
  if (!module_script) {
    // Step 9. "Let completion be Completion{[[Type]]: throw, [[Value]]: a new
    // TypeError, [[Target]]: empty}."
    // TODO(kouhei): show url?
    v8::Local<v8::Value> error = V8ThrowException::CreateTypeError(
        isolate, "Failed to fetch dynamically imported module.");

    // Step 10. "Perform FinishDynamicImport(referencingScriptOrModule,
    // specifier, promiseCapability, completion)."
    promise_resolver_->Reject(error);
    return;
  }

  // Step 11. "Otherwise," [spec text]
  // Step 13. "Run the module script module script, with the rethrow errors
  // flag set." [spec text]
  // TODO(domenic) There are no reference to "rethrow errors" in
  // #run-a-module-script
  module_script->RunScript();

  // Step 14. "If running the module script throws an exception, then" [spec
  // text]
  NOTIMPLEMENTED();
  if (false) {
    // Step 15. "Let completion be Completion{[[Type]]: throw, [[Value]]: the
    // thrown exception, [[Target]]: empty}." [spec text]
    NOTIMPLEMENTED();

    // Step 16. "Perform FinishDynamicImport(referencingScriptOrModule,
    // specifier, promiseCapability, completion)." [spec text]
    promise_resolver_->Reject(/* TODO(kouhei): fill error */);
  }

  // Step 17. "Otherwise, perform FinishDynamicImport(referencingScriptOrModule,
  // specifier, promiseCapability, NormalCompletion(undefined))." [spec text]
  ScriptModule record = module_script->Record();
  DCHECK(!record.IsNull());
  v8::Local<Value> record_value =
      record->NewLocal(isolate)->GetModuleNamespace();
  promise_resolver_->Resolve(record_value->GetModuleNamespace());
}

DEFINE_TRACE(ScriptModuleResolverImpl::DynamicResolver) {
  visitor->Trace(modulator_);
  visitor->Trace(promise_resolver_);
  ModuleTreeClient::Trace(visitor);
}

void ScriptModuleResolverImpl::ResolveDynamically(
    const String& specifier,
    const String& referrer,
    ScriptPromiseResolver* promise_resolver) {
  // https://github.com/tc39/proposal-dynamic-import/blob/master/HTML%20Integration.md#hostimportmoduledynamicallyreferencingscriptormodule-specifier-promisecapability
  // Step 1. "Let referencing script be
  // referencingScriptOrModule.[[HostDefined]]." [spec text]

  // Step 2. "Let settings object be referencing script's settings object."
  // [spec text] The settings object is modulator_, obtained from v8::Context
  // passed in to HostImportModuleDynamicallyInMainThread.

  // Step 3 at the end of this function.

  // Step 4. "Let url be the result of resolving a module specifier
  // given referencing script and specifier. If the result is failure,
  // asynchronously complete this algorithm with a TypeError exception and abort
  // these steps." [spec text]
  KURL referrer_url = KURL(NullURL(), referrer);
  if (!referrer_url.IsValid()) {
    v8::Isolate* isolate = modulator->GetScriptState()->GetIsolate();
    v8::Local<v8::Value> error = V8ThrowException::CreateTypeError(
        isolate, "Failed to resolve module specifier '" + specifier + "'");
    promise_resolver->Reject(error);
    return;
  }
  KURL url = Modulator::ResolveModuleSpecifier(specifier, referrer_url);

  String empty_nonce;
  // Step 5. "Let credentials mode be "omit"." [spec text]
  WebURLRequest::FetchCredentialsMode credentials_mode =
      WebURLRequest::kFetchCredentialsModeOmit;

  // Step 6. "If referencing script is a module script, set credentials mode to
  // referencing script's credentials mode." [spec text]
  ModuleScript* module_script = nullptr;
  {
    const auto it = record_to_module_script_map_.find(referrer);
    if (it != record_to_module_script_map_.end()) {
      module_script = it->value;
    }
  }
  if (module_script)
    credentials_mode = module_script->CredentialsMode();

  // Step 7. "Fetch a module script tree given url, settings object, "script",
  // ..." [spec text]

  // "referencing script's cryptographic nonce, ..." [spec text]
  String nonce;
  if (module_script) {
    nonce = module_script->Nonce();
  } else {
    NOTIMPLEMENTED()
        << "We need to obtain the nonce for referrer classic script somehow.";
  }

  // "referencing script's parser state, and ... " [spec text]
  ParserDisposition parser_state = kNotParserInserted;
  if (module_script) {
    parser_state = module_script->ParserState();
  } else {
    NOTIMPLEMENTED() << "We need to obtain the parser state for referrer "
                        "classic script somehow.";
  }

  // "credentials mode." [spec text]
  ModuleScriptFetchRequest request(url, empty_nonce, kNotParserInserted,
                                   credentials_mode);
  DynamicResolver* dynamic_resolver = DynamicResolver::Create(promise_resolver);
  modulator_->FetchTree(request, dynamic_resolver);

  // Steps 8-17 implemented at DynamicResolver::NotifyModuleLoadFinished.

  // Step 3. "Return undefined but continue running the following steps in
  // parallel:" [spec text]
}

void ScriptModuleResolverImpl::ContextDestroyed(ExecutionContext*) {
  // crbug.com/725816 : What we should really do is to make the map key
  // weak reference to v8::Module.
  record_to_module_script_map_.clear();
}

DEFINE_TRACE(ScriptModuleResolverImpl) {
  ScriptModuleResolver::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
  visitor->Trace(record_to_module_script_map_);
  visitor->Trace(modulator_);
}

}  // namespace blink
