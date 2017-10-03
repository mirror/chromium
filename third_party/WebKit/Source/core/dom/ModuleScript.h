// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ModuleScript_h
#define ModuleScript_h

#include "bindings/core/v8/ScriptModule.h"
#include "bindings/core/v8/ScriptValue.h"
#include "core/CoreExport.h"
#include "core/dom/Modulator.h"
#include "core/dom/Script.h"
#include "platform/bindings/ScriptWrappable.h"
#include "platform/bindings/TraceWrapperV8Reference.h"
#include "platform/heap/Handle.h"
#include "platform/loader/fetch/ResourceLoaderOptions.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/text/TextPosition.h"
#include "public/platform/WebURLRequest.h"

namespace blink {

// https://html.spec.whatwg.org/multipage/webappapis.html#concept-module-script-state
enum class ModuleInstantiationState {
  kUninstantiated,
  kErrored,
  kInstantiated,
};

const char* ModuleInstantiationStateToString(ModuleInstantiationState);

// ModuleScript is a model object for the "module script" spec concept.
// https://html.spec.whatwg.org/multipage/webappapis.html#module-script
class CORE_EXPORT ModuleScript final : public Script, public TraceWrapperBase {
 public:
  // https://html.spec.whatwg.org/#creating-a-module-script
  static ModuleScript* Create(
      const String& source_text,
      Modulator*,
      const KURL& base_url,
      const String& nonce,
      ParserDisposition,
      WebURLRequest::FetchCredentialsMode,
      AccessControlStatus,
      const TextPosition& start_position = TextPosition::MinimumPosition());

  // Mostly corresponds to Create() but accepts ScriptModule as the argument
  // and allows null ScriptModule.
  static ModuleScript* CreateForTest(Modulator*,
                                     ScriptModule,
                                     const KURL& base_url,
                                     const String& nonce,
                                     ParserDisposition,
                                     WebURLRequest::FetchCredentialsMode);

  ~ModuleScript() override = default;

  ScriptModule Record() const;
  bool HasEmptyRecord() const;
  const KURL& BaseURL() const { return base_url_; }

  ModuleInstantiationState State() const { return state_; }

  // Already removed from the spec.
  ScriptModuleState RecordStatusDeprecated() const;
  bool IsReadyForEvaluationForAssert() const;

  void SetParseErrorAndClearRecord(ScriptValue error);
  bool HasParseErrorForTest() const { return !parse_error_.IsEmpty(); }
  ScriptValue CreateParseError() const;

  void SetErrorToRethrow(ScriptValue error);
  bool HasErrorToRethrow() const { return !error_to_rethrow_.IsEmpty(); }
  ScriptValue CreateErrorToRethrow() const;

  ParserDisposition ParserState() const { return parser_state_; }
  WebURLRequest::FetchCredentialsMode CredentialsMode() const {
    return credentials_mode_;
  }
  const String& Nonce() const { return nonce_; }

  const TextPosition& StartPosition() const { return start_position_; }

  DECLARE_TRACE();
  DECLARE_TRACE_WRAPPERS();

 private:
  ModuleScript(Modulator* settings_object,
               ScriptModule record,
               const KURL& base_url,
               const String& nonce,
               ParserDisposition parser_state,
               WebURLRequest::FetchCredentialsMode credentials_mode,
               const String& source_text,
               const TextPosition& start_position);

  static ModuleScript* CreateInternal(const String& source_text,
                                      Modulator*,
                                      ScriptModule,
                                      const KURL& base_url,
                                      const String& nonce,
                                      ParserDisposition,
                                      WebURLRequest::FetchCredentialsMode,
                                      const TextPosition&);

  ScriptType GetScriptType() const override { return ScriptType::kModule; }
  bool CheckMIMETypeBeforeRunScript(Document* context_document,
                                    const SecurityOrigin*) const override;
  void RunScript(LocalFrame*, const SecurityOrigin*) const override;
  String InlineSourceTextForCSP() const override;

  friend class ModulatorImplBase;
  friend class ModuleTreeLinkerTestModulator;

  // https://html.spec.whatwg.org/multipage/webappapis.html#settings-object
  Member<Modulator> settings_object_;

  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-module-script-module-record
  TraceWrapperV8Reference<v8::Module> record_;

  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-module-script-base-url
  const KURL base_url_;

  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-module-script-instantiation-state
  ModuleInstantiationState state_ = ModuleInstantiationState::kUninstantiated;

  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-script-parse-error
  //
  // |record_|, |parse_error_| and |error_to_rethrow_| are TraceWrappers()ed and
  // kept alive via one or more of following reference graphs:
  // * non-inline module script case
  //   DOMWindow -> Modulator/ModulatorImpl -> ModuleMap -> ModuleMap::Entry
  //   -> ModuleScript
  // * inline module script case, before the PendingScript is created.
  //   DOMWindow -> Modulator/ModulatorImpl -> ModuleTreeLinkerRegistry
  //   -> ModuleTreeLinker -> ModuleScript
  // * inline module script case, after the PendingScript is created.
  //   HTMLScriptElement -> ScriptLoader -> ModulePendingScript
  //   -> ModulePendingScriptTreeClient -> ModuleScript.
  // * inline module script case, queued in HTMLParserScriptRunner,
  //   when HTMLScriptElement is removed before execution.
  //   Document -> HTMLDocumentParser -> HTMLParserScriptRunner
  //   -> ModulePendingScript -> ModulePendingScriptTreeClient
  //   -> ModuleScript.
  // * inline module script case, queued in ScriptRunner.
  //   Document -> ScriptRunner -> ScriptLoader -> ModulePendingScript
  //   -> ModulePendingScriptTreeClient -> ModuleScript.
  // All the classes/references on the graphs above should be
  // TraceWrapperBase/TraceWrapperMember<>/etc.,
  //
  // A parse error and an error to rethrow belong to a script, not to a
  // |parse_error_| and |error_to_rethrow_| should belong to a script (i.e.
  // blink::Script) according to the spec, but are put here in ModuleScript,
  // because:
  // - Error handling for classic and module scripts are somehow separated and
  //   there are no urgent motivation for merging the error handling and placing
  //   the errors in Script, and
  // - Classic scripts are handled according to the spec before
  //   https://github.com/whatwg/html/pull/2991. This shouldn't cause any
  //   observable functional changes, and updating the classic script handling
  //   will require moderate code changes (e.g. to move compilation timing).
  TraceWrapperV8Reference<v8::Value> parse_error_;

  // https://html.spec.whatwg.org/multipage/webappapis.html##concept-script-error-to-rethrow
  TraceWrapperV8Reference<v8::Value> error_to_rethrow_;

  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-module-script-nonce
  const String nonce_;

  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-module-script-parser
  const ParserDisposition parser_state_;

  // https://html.spec.whatwg.org/multipage/webappapis.html#concept-module-script-credentials-mode
  const WebURLRequest::FetchCredentialsMode credentials_mode_;

  // For CSP check.
  const String source_text_;

  const TextPosition start_position_;
};

CORE_EXPORT std::ostream& operator<<(std::ostream&, const ModuleScript&);

}  // namespace blink

#endif
