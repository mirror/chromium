// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/DynamicModuleResolver.h"

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

class DynamicModuleResolver::TreeClient : public ModuleTreeClient {
 public:
  static TreeClient* Create(const KURL& url,
                            Modulator* modulator,
                            ScriptPromiseResolver* promise_resolver) {
    return new TreeClient(url, modulator, promise_resolver);
  }

  DECLARE_TRACE();

 private:
  TreeClient(const KURL& url,
             Modulator* modulator,
             ScriptPromiseResolver* promise_resolver)
      : url_(url), modulator_(modulator), promise_resolver_(promise_resolver) {}

  // Implements ModuleTreeClient:
  void NotifyModuleTreeLoadFinished(ModuleScript*) final;

  KURL url_;
  Member<Modulator> modulator_;
  Member<ScriptPromiseResolver> promise_resolver_;
};

void DynamicModuleResolver::TreeClient::NotifyModuleTreeLoadFinished(
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
    // TODO(kouhei): show url?
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
  // TODO(domenic) There are no reference to "rethrow errors" in
  // #run-a-module-script
  modulator_->ExecuteModule(module_script);

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
  v8::Local<v8::Value> module_namespace = record.V8Namespace(isolate);
  promise_resolver_->Resolve(module_namespace);
}

DEFINE_TRACE(DynamicModuleResolver::TreeClient) {
  visitor->Trace(modulator_);
  visitor->Trace(promise_resolver_);
  ModuleTreeClient::Trace(visitor);
}

void DynamicModuleResolver::ResolveDynamically(
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
    v8::Isolate* isolate = modulator_->GetScriptState()->GetIsolate();
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
  ModuleScript* module_script =
      modulator_->GetFetchedModuleScript(referrer_url);
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
  TreeClient* dynamic_resolver =
      TreeClient::Create(url, modulator_.Get(), promise_resolver);
  modulator_->FetchTree(request, dynamic_resolver);

  // Steps 8-17 implemented at TreeClient::NotifyModuleLoadFinished.

  // Step 3. "Return undefined but continue running the following steps in
  // parallel:" [spec text]
}

}  // namespace blink
