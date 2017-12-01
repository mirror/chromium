/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTMLDocumentParser_h
#define HTMLDocumentParser_h

#include <memory>
#include "base/memory/scoped_refptr.h"
#include "core/CoreExport.h"
#include "core/dom/ParserContentPolicy.h"
#include "core/dom/ScriptableDocumentParser.h"
#include "core/html/parser/HTMLInputStream.h"
#include "core/html/parser/HTMLParserOptions.h"
#include "core/html/parser/HTMLParserReentryPermit.h"
#include "core/html/parser/HTMLParserScriptRunnerHost.h"
#include "core/html/parser/HTMLPreloadScanner.h"
#include "core/html/parser/HTMLSourceTracker.h"
#include "core/html/parser/HTMLToken.h"
#include "core/html/parser/HTMLTokenizer.h"
#include "core/html/parser/HTMLTreeBuilderSimulator.h"
#include "core/html/parser/PreloadRequest.h"
#include "core/html/parser/TextResourceDecoder.h"
#include "core/html/parser/XSSAuditor.h"
#include "core/html/parser/XSSAuditorDelegate.h"
#include "platform/bindings/TraceWrapperMember.h"
#include "platform/wtf/text/TextPosition.h"

namespace blink {

class CompactHTMLToken;
class Document;
class DocumentFragment;
class Element;
class HTMLDocument;
class HTMLParserScriptRunner;
class HTMLPreloadScanner;
class HTMLResourcePreloader;
class HTMLTreeBuilder;
class WebScheduler;

class CORE_EXPORT HTMLDocumentParser : public ScriptableDocumentParser,
                                       private HTMLParserScriptRunnerHost {
  USING_GARBAGE_COLLECTED_MIXIN(HTMLDocumentParser);

 public:
  static HTMLDocumentParser* Create(HTMLDocument& document) {
    return new HTMLDocumentParser(document);
  }
  ~HTMLDocumentParser() override;
  void Trace(blink::Visitor*) override;
  void TraceWrappers(const ScriptWrappableVisitor*) const override;

  static void ParseDocumentFragment(
      const String&,
      DocumentFragment*,
      Element* context_element,
      ParserContentPolicy = kAllowScriptingContent);

  // Exposed for testing.
  HTMLParserScriptRunnerHost* AsHTMLParserScriptRunnerHostForTesting() {
    return this;
  }

  HTMLTokenizer* Tokenizer() const { return tokenizer_.get(); }

  TextPosition GetTextPosition() const final;
  OrdinalNumber LineNumber() const final;

  HTMLParserReentryPermit* ReentryPermit() { return reentry_permit_.get(); }

  void AppendBytes(const char* bytes, size_t length) override;
  void Flush() final;
  void SetDecoder(std::unique_ptr<TextResourceDecoder>) final;

 protected:
  void insert(const String&) final;
  void Append(const String&) override;
  void Finish() final;

  explicit HTMLDocumentParser(HTMLDocument&);
  HTMLDocumentParser(DocumentFragment*,
                     Element* context_element,
                     ParserContentPolicy);

  HTMLTreeBuilder* TreeBuilder() const { return tree_builder_.Get(); }

  void ForcePlaintextForTextDocument();

 private:
  static HTMLDocumentParser* Create(DocumentFragment* fragment,
                                    Element* context_element,
                                    ParserContentPolicy parser_content_policy) {
    return new HTMLDocumentParser(fragment, context_element,
                                  parser_content_policy);
  }
  HTMLDocumentParser(Document&, ParserContentPolicy);

  // DocumentParser
  void Detach() final;
  bool HasInsertionPoint() final;
  void PrepareToStopParsing() final;
  void StopParsing() final;
  bool IsPaused() const {
    return IsWaitingForScripts() || is_waiting_for_stylesheets_;
  }
  bool IsWaitingForScripts() const final;
  bool IsExecutingScript() const final;
  void ExecuteScriptsWaitingForResources() final;
  void DidAddPendingStylesheetInBody() final;
  void DidLoadAllBodyStylesheets() final;
  void CheckIfBodyStylesheetAdded();
  void DocumentElementAvailable() override;

  // HTMLParserScriptRunnerHost
  void NotifyScriptLoaded(PendingScript*) final;
  HTMLInputStream& InputStream() final { return input_; }
  bool HasPreloadScanner() const final { return preload_scanner_.get(); }
  void AppendCurrentInputStreamToPreloadScannerAndScan() final;

  bool CanTakeNextToken();
  void PumpTokenizer();
  void PumpTokenizerIfPossible();
  void ConstructTreeFromHTMLToken();
  void ConstructTreeFromCompactHTMLToken(const CompactHTMLToken&);

  void RunScriptsForPausedTreeBuilder();
  void ResumeParsingAfterPause();

  void AttemptToEnd();
  void EndIfDelayed();
  void AttemptToRunDeferredScriptsAndEnd();
  void end();

  bool IsParsingFragment() const;
  bool InPumpSession() const { return pump_session_nesting_level_ > 0; }
  bool ShouldDelayEnd() const {
    return InPumpSession() || IsPaused() || IsExecutingScript() ||
           resume_task_handle_.IsActive();
  }

  std::unique_ptr<HTMLPreloadScanner> CreatePreloadScanner(
      TokenPreloadScanner::ScannerType);

  // Let the given HTMLPreloadScanner scan the input it has, and then preloads
  // resources using the resulting PreloadRequests and |m_preloader|.
  void ScanAndPreload(HTMLPreloadScanner*);
  void FetchQueuedPreloads();

  void EnsureTokenPreloadScanner();

  HTMLToken& Token() { return *token_; }

  HTMLParserOptions options_;
  HTMLInputStream input_;
  scoped_refptr<HTMLParserReentryPermit> reentry_permit_;

  std::unique_ptr<HTMLToken> token_;
  std::unique_ptr<HTMLTokenizer> tokenizer_;
  TraceWrapperMember<HTMLParserScriptRunner> script_runner_;
  Member<HTMLTreeBuilder> tree_builder_;

  std::unique_ptr<TokenPreloadScanner> token_preload_scanner_;

  std::unique_ptr<HTMLPreloadScanner> preload_scanner_;
  // A scanner used only for input provided to the insert() method.
  std::unique_ptr<HTMLPreloadScanner> insertion_preload_scanner_;

  PreloadRequestStream queued_preloads_;

  // If we had to yield for a high priority task, this handle will be active.
  TaskHandle resume_task_handle_;
  scoped_refptr<WebTaskRunner> loading_task_runner_;
  HTMLSourceTracker source_tracker_;
  TextPosition text_position_;
  XSSAuditor xss_auditor_;
  XSSAuditorDelegate xss_auditor_delegate_;

  Member<HTMLResourcePreloader> preloader_;

  bool end_was_delayed_;
  unsigned pump_session_nesting_level_;
  bool added_pending_stylesheet_in_body_;
  bool is_waiting_for_stylesheets_;

  WebScheduler* web_scheduler_;  // NOT OWNED.
};

}  // namespace blink

#endif  // HTMLDocumentParser_h
