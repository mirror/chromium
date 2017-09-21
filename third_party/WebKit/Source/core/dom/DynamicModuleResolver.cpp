// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/DynamicModuleResolver.h"

#include "bindings/core/v8/ReferrerScriptInfo.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/dom/Modulator.h"
#include "core/dom/ModuleScript.h"
#include "core/loader/modulescript/ModuleScriptFetchRequest.h"
#include "platform/bindings/V8ThrowException.h"
#include "v8/include/v8.h"

namespace blink {

DEFINE_TRACE(DynamicModuleResolver) {
  visitor->Trace(modulator_);
}

namespace {

class DynamicImportTreeClient final : public ModuleTreeClient {
 public:
  static DynamicImportTreeClient* Create(
      const KURL& url,
      Modulator* modulator,
      ScriptPromiseResolver* promise_resolver) {
    return new DynamicImportTreeClient(url, modulator, promise_resolver);
  }

  DECLARE_TRACE();

 private:
  DynamicImportTreeClient(const KURL& url,
                          Modulator* modulator,
                          ScriptPromiseResolver* promise_resolver)
      : url_(url), modulator_(modulator), promise_resolver_(promise_resolver) {}

  // Implements ModuleTreeClient:
  void NotifyModuleTreeLoadFinished(ModuleScript*) final;

  const KURL url_;
  const Member<Modulator> modulator_;
  const Member<ScriptPromiseResolver> promise_resolver_;
};

void DynamicImportTreeClient::NotifyModuleTreeLoadFinished(
    ModuleScript* module_script) {
  DCHECK(modulator_->HasValidContext());
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
    v8::Local<v8::Value> error = V8ThrowException::CreateTypeError(
        isolate,
        "Failed to fetch dynamically imported module: " + url_.GetString());

    // Step 10. "Perform FinishDynamicImport(referencingScriptOrModule,
    // specifier, promiseCapability, completion)."
    promise_resolver_->Reject(error);
    return;
  }

  // Step 11. "Otherwise," [spec text]
  // Step 12 is at top of this function.
  // Step 13. "Run the module script module script, with the rethrow errors
  // flag set." [spec text]
  // TODO(domenic): There are no reference to "rethrow errors" in
  // #run-a-module-script
  modulator_->ExecuteModule(module_script);

  // Step 14. "If running the module script throws an exception, then" [spec
  // text]
  if (module_script->IsErrored()) {
    // Step 15. "Let completion be Completion{[[Type]]: throw, [[Value]]: the
    // thrown exception, [[Target]]: empty}." [spec text]
    // TODO(domenic): Align spec to use the "module script"'s error.
    ScriptValue error = modulator_->GetError(module_script);

    // Step 16. "Perform FinishDynamicImport(referencingScriptOrModule,
    // specifier, promiseCapability, completion)." [spec text]
    promise_resolver_->Reject(error);
    return;
  }

  // Step 17. "Otherwise, perform FinishDynamicImport(referencingScriptOrModule,
  // specifier, promiseCapability, NormalCompletion(undefined))." [spec text]
  ScriptModule record = module_script->Record();
  DCHECK(!record.IsNull());

  v8::Local<v8::Value> module_namespace = record.V8Namespace(isolate);
  promise_resolver_->Resolve(module_namespace);
}

DEFINE_TRACE(DynamicImportTreeClient) {
  visitor->Trace(modulator_);
  visitor->Trace(promise_resolver_);
  ModuleTreeClient::Trace(visitor);
}

}  // namespace

void DynamicModuleResolver::ResolveDynamically(
    const String& specifier,
    const String& referrer_url_str,
    const ReferrerScriptInfo& referrer_info,
    ScriptPromiseResolver* promise_resolver) {
  DCHECK(modulator_->GetScriptState()->GetIsolate()->InContext())
      << "ResolveDynamically should be called from V8 callback, within a valid "
         "context.";

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
  KURL referrer_url = KURL(NullURL(), referrer_url_str);
  DCHECK(referrer_url.IsValid());
  KURL url = Modulator::ResolveModuleSpecifier(specifier, referrer_url);
  if (!url.IsValid()) {
    v8::Isolate* isolate = modulator_->GetScriptState()->GetIsolate();
    v8::Local<v8::Value> error = V8ThrowException::CreateTypeError(
        isolate, "Failed to resolve module specifier '" + specifier + "'");
    promise_resolver->Reject(error);
    return;
  }

  // Step 5. "Let credentials mode be "omit"." [spec text]
  // Note: This is set as the ReferrerScriptInfo's default value. See
  // ReferrerScriptInfo.h. Step 6. "If referencing script is a module script,
  // set credentials mode to referencing script's credentials mode." [spec text]
  WebURLRequest::FetchCredentialsMode credentials_mode =
      referrer_info.CredentialsMode();

  // Step 7. "Fetch a module script tree given url, settings object, "script",
  // referencing script's cryptographic nonce, referencing script's parser
  // state, and credentials mode." [spec text]
  const String& nonce = referrer_info.Nonce();
  ParserDisposition parser_state = referrer_info.ParserState();
  ModuleScriptFetchRequest request(url, nonce, parser_state, credentials_mode);
  auto tree_client =
      DynamicImportTreeClient::Create(url, modulator_.Get(), promise_resolver);
  modulator_->FetchTree(request, tree_client);

  // Steps 8-17 implemented at
  // DynamicImportTreeClient::NotifyModuleLoadFinished.

  // Step 3. "Return undefined but continue running the following steps in
  // parallel:" [spec text]
}

}  // namespace blink
