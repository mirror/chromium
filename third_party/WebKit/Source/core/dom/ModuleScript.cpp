// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ModuleScript.h"

#include "bindings/core/v8/ScriptValue.h"
#include "core/dom/ScriptModuleResolver.h"
#include "platform/bindings/ScriptState.h"
#include "v8/include/v8.h"

namespace blink {

const char* ModuleInstantiationStateToString(ModuleInstantiationState state) {
  switch (state) {
    case ModuleInstantiationState::kUninstantiated:
      return "uninstantiated";
    case ModuleInstantiationState::kInstantiated:
      return "instantiated";
    case ModuleInstantiationState::kErrored:
      return "errored";
  }
  NOTREACHED();
  return "";
}

ModuleScript* ModuleScript::Create(
    const String& source_text,
    Modulator* modulator,
    const KURL& base_url,
    const String& nonce,
    ParserDisposition parser_state,
    WebURLRequest::FetchCredentialsMode credentials_mode,
    AccessControlStatus access_control_status,
    const TextPosition& start_position) {
  // https://html.spec.whatwg.org/#creating-a-module-script

  // Step 1. "If scripting is disabled for settings's responsible browsing
  // context, then set source to the empty string." [spec text]
  //
  // TODO: Implement this?

  // Step 2. "Let script be a new module script that this algorithm will
  // subsequently initialize." [spec text]

  // Step 3. "Set script's settings object to settings." [spec text]
  //
  // Note: "script's settings object" will be |modulator|.

  // Delegate to Modulator::CompileModule to process Steps 5-6.
  ScriptState* script_state = modulator->GetScriptState();
  ScriptState::Scope scope(script_state);
  v8::Isolate* isolate = script_state->GetIsolate();
  ExceptionState exception_state(isolate, ExceptionState::kExecutionContext,
                                 "ModuleScript", "Create");
  ScriptModule result = modulator->CompileModule(
      source_text, base_url.GetString(), access_control_status,
      credentials_mode, nonce, parser_state, start_position, exception_state);

  // CreateInternal processes Steps 4 and 8-13.
  //
  // [nospec] We initialize the other ModuleScript members anyway by running
  // Steps 8-13 before Step 6. In a case that compile failed, we will
  // immediately turn the script into errored state. Thus the members will not
  // be used for the speced algorithms, but may be used from inspector.
  ModuleScript* script =
      CreateInternal(source_text, modulator, result, base_url, nonce,
                     parser_state, credentials_mode, start_position);

  // Step 6. "If result is a list of errors, then:" [spec text]
  if (exception_state.HadException()) {
    DCHECK(result.IsNull());

    // Step 6.1. "Set script's parse error to result[0]." [spec text]
    v8::Local<v8::Value> error = exception_state.GetException();
    exception_state.ClearException();
    script->SetParseErrorAndClearRecord(ScriptValue(script_state, error));

    // Step 6.2. "Return script." [spec text]
    return script;
  }

  // Step 7. "For each string requested of record.[[RequestedModules]]:" [spec
  // text]
  for (const auto& requested :
       modulator->ModuleRequestsFromScriptModule(result)) {
    // Step 7.1. "Let url be the result of resolving a module specifier given
    // module script and requested."[spec text]
    //
    // Step 7.2. "If url is failure:" [spec text]
    //
    // TODO(kouhei): Cache the url here instead of issuing
    // ResolveModuleSpecifier later again in ModuleTreeLinker.
    if (modulator->ResolveModuleSpecifier(requested.specifier, base_url)
            .IsValid())
      continue;

    // Step 7.2.1. "Let error be a new TypeError exception." [spec text]
    v8::Local<v8::Value> error = V8ThrowException::CreateTypeError(
        isolate,
        "Failed to resolve module specifier '" + requested.specifier + "'");

    // Step 7.2.2. "Set script's parse error to error." [spec text]
    script->SetParseErrorAndClearRecord(ScriptValue(script_state, error));

    // Step 7.2.3. "Return script." [spec text]
    return script;
  }

  return script;
}

ModuleScript* ModuleScript::CreateForTest(
    Modulator* modulator,
    ScriptModule record,
    const KURL& base_url,
    const String& nonce,
    ParserDisposition parser_state,
    WebURLRequest::FetchCredentialsMode credentials_mode) {
  String dummy_source_text = "";
  return CreateInternal(dummy_source_text, modulator, record, base_url, nonce,
                        parser_state, credentials_mode,
                        TextPosition::MinimumPosition());
}

ModuleScript* ModuleScript::CreateInternal(
    const String& source_text,
    Modulator* modulator,
    ScriptModule result,
    const KURL& base_url,
    const String& nonce,
    ParserDisposition parser_state,
    WebURLRequest::FetchCredentialsMode credentials_mode,
    const TextPosition& start_position) {
  // https://html.spec.whatwg.org/#creating-a-module-script
  // Step 4. Set script's parse error and error to rethrow to null.
  // Step 8. Set script's module record to result.
  // Step 9. Set script's base URL to the script base URL provided.
  // Step 10. Set script's cryptographic nonce to the cryptographic nonce
  // provided.
  // Step 11. Set script's parser state to the parser state.
  // Step 12. Set script's credentials mode to the credentials mode provided.
  // Step 13. Return script.
  // [not specced] |source_text| is saved for CSP checks.
  ModuleScript* module_script =
      new ModuleScript(modulator, result, base_url, nonce, parser_state,
                       credentials_mode, source_text, start_position);

  // Step 5, a part of ParseModule(): Passing script as the last parameter
  // here ensures result.[[HostDefined]] will be script.
  modulator->GetScriptModuleResolver()->RegisterModuleScript(module_script);

  return module_script;
}

ModuleScript::ModuleScript(Modulator* settings_object,
                           ScriptModule record,
                           const KURL& base_url,
                           const String& nonce,
                           ParserDisposition parser_state,
                           WebURLRequest::FetchCredentialsMode credentials_mode,
                           const String& source_text,
                           const TextPosition& start_position)
    : settings_object_(settings_object),
      record_(this),
      base_url_(base_url),
      parse_error_(this),
      error_to_rethrow_(this),
      nonce_(nonce),
      parser_state_(parser_state),
      credentials_mode_(credentials_mode),
      source_text_(source_text),
      start_position_(start_position) {
  if (record.IsNull()) {
    // We allow empty records for module infra tests which never touch records.
    // This should never happen outside unit tests.
    return;
  }

  DCHECK(settings_object);
  v8::Isolate* isolate = settings_object_->GetScriptState()->GetIsolate();
  v8::HandleScope scope(isolate);
  record_.Set(isolate, record.NewLocal(isolate));
}

ScriptModule ModuleScript::Record() const {
  if (record_.IsEmpty())
    return ScriptModule();

  v8::Isolate* isolate = settings_object_->GetScriptState()->GetIsolate();
  v8::HandleScope scope(isolate);
  return ScriptModule(isolate, record_.NewLocal(isolate));
}

bool ModuleScript::HasEmptyRecord() const {
  return record_.IsEmpty();
}

ScriptModuleState ModuleScript::RecordStatusDeprecated() const {
  DCHECK(!record_.IsEmpty());

  ScriptState* script_state = settings_object_->GetScriptState();
  ScriptState::Scope scope(script_state);
  return Record().Status(script_state);
}

bool ModuleScript::HasInstantiatedDeprecated() const {
  // "We say that a module script has instantiated if ..." [spec text]

  // "its module record is not null, and ..." [spec text]
  if (record_.IsEmpty())
    return false;

  // "its module record's [[Status]] field is ..." [spec text]
  ScriptModuleState status = RecordStatusDeprecated();

  // "either "instantiated" or "evaluated"." [spec text]
  return status == ScriptModuleState::kInstantiated ||
         status == ScriptModuleState::kEvaluated;
}

bool ModuleScript::IsErroredDeprecated() const {
  // "We say that a module script is errored ..." [spec text]

  // "if either its module record is null, ..." [spec text]
  if (record_.IsEmpty())
    return true;

  // "or its module record's [[Status]] field has the value "errored"." [spec
  // text]
  return RecordStatusDeprecated() == ScriptModuleState::kErrored;
}

void ModuleScript::SetParseErrorAndClearRecord(ScriptValue error) {
  DCHECK(!record_.IsEmpty());
  DCHECK(!error.IsEmpty());

  record_.Clear();
  ScriptState::Scope scope(error.GetScriptState());
  parse_error_.Set(error.GetIsolate(), error.V8Value());
}

ScriptValue ModuleScript::CreateParseError() const {
  ScriptState* script_state = settings_object_->GetScriptState();
  v8::Isolate* isolate = script_state->GetIsolate();
  ScriptState::Scope scope(script_state);
  ScriptValue error(script_state, parse_error_.NewLocal(isolate));
  DCHECK(!error.IsEmpty());
  return error;
}

void ModuleScript::SetErrorToRethrow(ScriptValue error) {
  ScriptState::Scope scope(error.GetScriptState());
  error_to_rethrow_.Set(error.GetIsolate(), error.V8Value());
}

ScriptValue ModuleScript::CreateErrorToRethrow() const {
  ScriptState* script_state = settings_object_->GetScriptState();
  v8::Isolate* isolate = script_state->GetIsolate();
  ScriptState::Scope scope(script_state);
  ScriptValue error(script_state, error_to_rethrow_.NewLocal(isolate));
  DCHECK(!error.IsEmpty());
  return error;
}

DEFINE_TRACE(ModuleScript) {
  visitor->Trace(settings_object_);
  Script::Trace(visitor);
}
DEFINE_TRACE_WRAPPERS(ModuleScript) {
  // TODO(mlippautz): Support TraceWrappers(const
  // TraceWrapperV8Reference<v8::Module>&) to remove the cast.
  visitor->TraceWrappers(record_.UnsafeCast<v8::Value>());
  visitor->TraceWrappers(parse_error_);
  visitor->TraceWrappers(error_to_rethrow_);
}

bool ModuleScript::CheckMIMETypeBeforeRunScript(Document* context_document,
                                                const SecurityOrigin*) const {
  // We don't check MIME type here because we check the MIME type in
  // ModuleScriptLoader::WasModuleLoadSuccessful().
  return true;
}

void ModuleScript::RunScript(LocalFrame* frame, const SecurityOrigin*) const {
  DVLOG(1) << *this << "::RunScript()";
  settings_object_->ExecuteModule(this);
}

String ModuleScript::InlineSourceTextForCSP() const {
  return source_text_;
}

std::ostream& operator<<(std::ostream& stream,
                         const ModuleScript& module_script) {
  stream << "ModuleScript[" << &module_script << ", ";
  if (module_script.HasEmptyRecord()) {
    stream << "errored (empty record)";
  } else {
    stream << "record's [[Status]] = "
           << ScriptModuleStateToString(module_script.RecordStatusDeprecated());
  }

  return stream << "]";
}

}  // namespace blink
