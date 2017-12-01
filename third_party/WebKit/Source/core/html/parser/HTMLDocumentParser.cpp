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

#include "core/html/parser/HTMLDocumentParser.h"

#include <memory>
#include "core/css/MediaValuesCached.h"
#include "core/css/resolver/StyleResolver.h"
#include "core/dom/DocumentFragment.h"
#include "core/dom/Element.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/Settings.h"
#include "core/html/HTMLDocument.h"
#include "core/html/parser/AtomicHTMLToken.h"
#include "core/html/parser/HTMLParserScriptRunner.h"
#include "core/html/parser/HTMLResourcePreloader.h"
#include "core/html/parser/HTMLTreeBuilder.h"
#include "core/html/parser/NestingLevelIncrementer.h"
#include "core/html_names.h"
#include "core/inspector/InspectorTraceEvents.h"
#include "core/loader/DocumentLoader.h"
#include "core/loader/LinkLoader.h"
#include "core/loader/NavigationScheduler.h"
#include "core/probe/CoreProbes.h"
#include "platform/CrossThreadFunctional.h"
#include "platform/Histogram.h"
#include "platform/SharedBuffer.h"
#include "platform/WebFrameScheduler.h"
#include "platform/bindings/RuntimeCallStats.h"
#include "platform/bindings/V8PerIsolateData.h"
#include "platform/heap/Handle.h"
#include "platform/instrumentation/tracing/TraceEvent.h"
#include "platform/loader/fetch/ResourceFetcher.h"
#include "platform/scheduler/child/web_scheduler.h"
#include "platform/wtf/AutoReset.h"
#include "platform/wtf/PtrUtil.h"
#include "public/platform/Platform.h"
#include "public/platform/TaskType.h"
#include "public/platform/WebLoadingBehaviorFlag.h"
#include "public/platform/WebThread.h"

namespace blink {

namespace {
class PumpSession : public NestingLevelIncrementer {
  STACK_ALLOCATED();

 public:
  PumpSession(unsigned& nesting_level);
  ~PumpSession();
};

PumpSession::PumpSession(unsigned& nesting_level)
    : NestingLevelIncrementer(nesting_level) {}

PumpSession::~PumpSession() {}
}  // namespace

using namespace HTMLNames;

// This is a direct transcription of step 4 from:
// http://www.whatwg.org/specs/web-apps/current-work/multipage/the-end.html#fragment-case
static HTMLTokenizer::State TokenizerStateForContextElement(
    Element* context_element,
    bool report_errors,
    const HTMLParserOptions& options) {
  if (!context_element)
    return HTMLTokenizer::kDataState;

  const QualifiedName& context_tag = context_element->TagQName();

  if (context_tag.Matches(titleTag) || context_tag.Matches(textareaTag))
    return HTMLTokenizer::kRCDATAState;
  if (context_tag.Matches(styleTag) || context_tag.Matches(xmpTag) ||
      context_tag.Matches(iframeTag) ||
      (context_tag.Matches(noembedTag) && options.plugins_enabled) ||
      (context_tag.Matches(noscriptTag) && options.script_enabled) ||
      context_tag.Matches(noframesTag))
    return report_errors ? HTMLTokenizer::kRAWTEXTState
                         : HTMLTokenizer::kPLAINTEXTState;
  if (context_tag.Matches(scriptTag))
    return report_errors ? HTMLTokenizer::kScriptDataState
                         : HTMLTokenizer::kPLAINTEXTState;
  if (context_tag.Matches(plaintextTag))
    return HTMLTokenizer::kPLAINTEXTState;
  return HTMLTokenizer::kDataState;
}

HTMLDocumentParser::HTMLDocumentParser(HTMLDocument& document)
    : HTMLDocumentParser(document, kAllowScriptingContent) {
  script_runner_ =
      HTMLParserScriptRunner::Create(ReentryPermit(), &document, this);
  tree_builder_ =
      HTMLTreeBuilder::Create(this, document, kAllowScriptingContent, options_);
}

HTMLDocumentParser::HTMLDocumentParser(
    DocumentFragment* fragment,
    Element* context_element,
    ParserContentPolicy parser_content_policy)
    : HTMLDocumentParser(fragment->GetDocument(), parser_content_policy) {
  // No m_scriptRunner in fragment parser.
  tree_builder_ = HTMLTreeBuilder::Create(this, fragment, context_element,
                                          parser_content_policy, options_);

  // For now document fragment parsing never reports errors.
  bool report_errors = false;
  tokenizer_->SetState(TokenizerStateForContextElement(
      context_element, report_errors, options_));
  xss_auditor_.InitForFragment();
}

HTMLDocumentParser::HTMLDocumentParser(Document& document,
                                       ParserContentPolicy content_policy)
    : ScriptableDocumentParser(document, content_policy),
      options_(&document),
      reentry_permit_(HTMLParserReentryPermit::Create()),
      token_(WTF::WrapUnique(new HTMLToken)),
      tokenizer_(HTMLTokenizer::Create(options_)),
      loading_task_runner_(document.GetTaskRunner(TaskType::kNetworking)),
      xss_auditor_delegate_(&document),
      preloader_(HTMLResourcePreloader::Create(document)),
      end_was_delayed_(false),
      pump_session_nesting_level_(0),
      added_pending_stylesheet_in_body_(false),
      is_waiting_for_stylesheets_(false),
      web_scheduler_(Platform::Current()->CurrentThread()->Scheduler()) {
  DCHECK(token_ && tokenizer_);
}

HTMLDocumentParser::~HTMLDocumentParser() {}

void HTMLDocumentParser::Trace(blink::Visitor* visitor) {
  visitor->Trace(tree_builder_);
  visitor->Trace(xss_auditor_delegate_);
  visitor->Trace(script_runner_);
  visitor->Trace(preloader_);
  ScriptableDocumentParser::Trace(visitor);
  HTMLParserScriptRunnerHost::Trace(visitor);
}

void HTMLDocumentParser::TraceWrappers(
    const ScriptWrappableVisitor* visitor) const {
  visitor->TraceWrappers(script_runner_);
}

void HTMLDocumentParser::Detach() {
  DocumentParser::Detach();
  if (script_runner_)
    script_runner_->Detach();
  tree_builder_->Detach();
  // FIXME: It seems wrong that we would have a preload scanner here. Yet during
  // fast/dom/HTMLScriptElement/script-load-events.html we do.
  preload_scanner_.reset();
  insertion_preload_scanner_.reset();
  // Oilpan: It is important to clear m_token to deallocate backing memory of
  // HTMLToken::m_data and let the allocator reuse the memory for
  // HTMLToken::m_data of a next HTMLDocumentParser. We need to clear
  // m_tokenizer first because m_tokenizer has a raw pointer to m_token.
  tokenizer_.reset();
  token_.reset();
}

void HTMLDocumentParser::StopParsing() {
  DocumentParser::StopParsing();
}

// This kicks off "Once the user agent stops parsing" as described by:
// http://www.whatwg.org/specs/web-apps/current-work/multipage/the-end.html#the-end
void HTMLDocumentParser::PrepareToStopParsing() {
  // That means hasInsertionPoint() may not be correct in some cases.
  DCHECK(!HasInsertionPoint());

  // NOTE: This pump should only ever emit buffered character tokens.
  PumpTokenizerIfPossible();

  if (IsStopped())
    return;

  DocumentParser::PrepareToStopParsing();

  // We will not have a scriptRunner when parsing a DocumentFragment.
  if (script_runner_)
    GetDocument()->SetReadyState(Document::kInteractive);

  // Setting the ready state above can fire mutation event and detach us from
  // underneath. In that case, just bail out.
  if (IsDetached())
    return;

  AttemptToRunDeferredScriptsAndEnd();
}

bool HTMLDocumentParser::IsParsingFragment() const {
  return tree_builder_->IsParsingFragment();
}

void HTMLDocumentParser::PumpTokenizerIfPossible() {
  CheckIfBodyStylesheetAdded();
  if (IsStopped() || IsPaused())
    return;

  resume_task_handle_.Cancel();
  PumpTokenizer();
}

void HTMLDocumentParser::RunScriptsForPausedTreeBuilder() {
  DCHECK(ScriptingContentIsAllowed(GetParserContentPolicy()));

  TextPosition script_start_position = TextPosition::BelowRangePosition();
  Element* script_element =
      tree_builder_->TakeScriptToProcess(script_start_position);
  // We will not have a scriptRunner when parsing a DocumentFragment.
  if (script_runner_)
    script_runner_->ProcessScriptElement(script_element, script_start_position);
  CheckIfBodyStylesheetAdded();
}

bool HTMLDocumentParser::CanTakeNextToken() {
  if (IsStopped())
    return false;

  // If we're paused waiting for a script, we try to execute scripts before
  // continuing.
  if (tree_builder_->HasParserBlockingScript())
    RunScriptsForPausedTreeBuilder();
  if (IsStopped() || IsPaused())
    return false;

  // FIXME: It's wrong for the HTMLDocumentParser to reach back to the
  // LocalFrame, but this approach is how the old parser handled stopping when
  // the page assigns window.location.  What really should happen is that
  // assigning window.location causes the parser to stop parsing cleanly.  The
  // problem is we're not perpared to do that at every point where we run
  // JavaScript.
  if (!IsParsingFragment() && GetDocument()->GetFrame() &&
      GetDocument()
          ->GetFrame()
          ->GetNavigationScheduler()
          .LocationChangePending())
    return false;

  return true;
}

void HTMLDocumentParser::ForcePlaintextForTextDocument() {
  EnsureTokenPreloadScanner();
  tokenizer_->SetState(HTMLTokenizer::kPLAINTEXTState);
}

void HTMLDocumentParser::EnsureTokenPreloadScanner() {
  DCHECK(GetDocument());

  // Make sure that a resolver is set up, so that the correct viewport
  // dimensions will be fed to the preload scanner.
  if (GetDocument()->Loader())
    GetDocument()->EnsureStyleResolver();

  if (!token_preload_scanner_) {
    token_preload_scanner_.reset(new TokenPreloadScanner(
        GetDocument()->Url(), CachedDocumentParameters::Create(GetDocument()),
        MediaValuesCached::MediaValuesCachedData(*GetDocument()),
        TokenPreloadScanner::ScannerType::kMainDocument));
  }
}

void HTMLDocumentParser::PumpTokenizer() {
  DCHECK(!IsStopped());
  DCHECK(tokenizer_);
  DCHECK(token_);

  PumpSession session(pump_session_nesting_level_);

  // We tell the InspectorInstrumentation about every pump, even if we end up
  // pumping nothing.  It can filter out empty pumps itself.
  // FIXME: m_input.current().length() is only accurate if we end up parsing the
  // whole buffer in this pump.  We should pass how much we parsed as part of
  // didWriteHTML instead of willWriteHTML.
  probe::ParseHTML probe(GetDocument(), this);

  if (!IsParsingFragment())
    xss_auditor_.Init(GetDocument(), &xss_auditor_delegate_);

  bool should_yeild = false;
  while (CanTakeNextToken() && !should_yeild) {
    if (xss_auditor_.IsEnabled())
      source_tracker_.Start(input_.Current(), tokenizer_.get(), Token());

    {
      RUNTIME_CALL_TIMER_SCOPE(
          V8PerIsolateData::MainThreadIsolate(),
          RuntimeCallStats::CounterId::kHTMLTokenizerNextToken);
      if (!tokenizer_->NextToken(input_.Current(), Token()))
        break;
    }

    if (xss_auditor_.IsEnabled()) {
      source_tracker_.end(input_.Current(), tokenizer_.get(), Token());

      // We do not XSS filter innerHTML, which means we (intentionally) fail
      // http/tests/security/xssAuditor/dom-write-innerHTML.html
      if (std::unique_ptr<XSSInfo> xss_info =
              xss_auditor_.FilterToken(FilterTokenRequest(
                  Token(), source_tracker_, tokenizer_->ShouldAllowCDATA()))) {
        xss_auditor_delegate_.DidBlockScript(*xss_info);
        // If we're in blocking mode, we might stop the parser in
        // 'didBlockScript()'. In that case, exit early.
        if (!IsParsing())
          return;
      }
    }

    if (token_preload_scanner_) {
      bool is_csp_meta_tag = false;
      token_preload_scanner_->Scan(Token(), input_.Current(), queued_preloads_,
                                   nullptr, &is_csp_meta_tag);
      FetchQueuedPreloads();
    }

    ConstructTreeFromHTMLToken();
    DCHECK(IsStopped() || Token().IsUninitialized());

    // Don't hog the thread if there's something more important to do.
    should_yeild = web_scheduler_->ShouldYieldForHighPriorityWork();
  }

  if (IsStopped()) {
    return;
  }

  // There should only be PendingText left since the tree-builder always flushes
  // the task queue before returning. In case that ever changes, crash.
  tree_builder_->Flush(kFlushAlways);
  CHECK(!IsStopped());

  if (IsPaused()) {
    DCHECK_EQ(tokenizer_->GetState(), HTMLTokenizer::kDataState);

    DCHECK(preloader_);
    // TODO(kouhei): m_preloader should be always available for synchronous
    // parsing case, adding paranoia if for speculative crash fix for
    // crbug.com/465478
    if (preloader_) {
      if (!preload_scanner_) {
        preload_scanner_ = CreatePreloadScanner(
            TokenPreloadScanner::ScannerType::kMainDocument);
        preload_scanner_->AppendToEnd(input_.Current());
      }
      ScanAndPreload(preload_scanner_.get());
    }
  } else if (should_yeild) {
    resume_task_handle_ = loading_task_runner_->PostCancellableTask(
        BLINK_FROM_HERE, WTF::Bind(&HTMLDocumentParser::PumpTokenizerIfPossible,
                                   WrapPersistent(this)));
  }
}

void HTMLDocumentParser::ConstructTreeFromHTMLToken() {
  AtomicHTMLToken atomic_token(Token());

  // We clear the m_token in case constructTreeFromAtomicToken
  // synchronously re-enters the parser. We don't clear the token immedately
  // for Character tokens because the AtomicHTMLToken avoids copying the
  // characters by keeping a pointer to the underlying buffer in the
  // HTMLToken. Fortunately, Character tokens can't cause us to re-enter
  // the parser.
  //
  // FIXME: Stop clearing the m_token once we start running the parser off
  // the main thread or once we stop allowing synchronous JavaScript
  // execution from parseAttribute.
  if (Token().GetType() != HTMLToken::kCharacter)
    Token().Clear();

  tree_builder_->ConstructTree(&atomic_token);
  CheckIfBodyStylesheetAdded();

  // FIXME: constructTree may synchronously cause Document to be detached.
  if (!token_)
    return;

  if (!Token().IsUninitialized()) {
    DCHECK_EQ(Token().GetType(), HTMLToken::kCharacter);
    Token().Clear();
  }
}

void HTMLDocumentParser::ConstructTreeFromCompactHTMLToken(
    const CompactHTMLToken& compact_token) {
  AtomicHTMLToken token(compact_token);
  tree_builder_->ConstructTree(&token);
  CheckIfBodyStylesheetAdded();
}

bool HTMLDocumentParser::HasInsertionPoint() {
  // FIXME: The wasCreatedByScript() branch here might not be fully correct. Our
  // model of the EOF character differs slightly from the one in the spec
  // because our treatment is uniform between network-sourced and script-sourced
  // input streams whereas the spec treats them differently.
  return input_.HasInsertionPoint() ||
         (WasCreatedByScript() && !input_.HaveSeenEndOfFile());
}

void HTMLDocumentParser::insert(const String& source) {
  if (IsStopped())
    return;

  TRACE_EVENT1("blink", "HTMLDocumentParser::insert", "source_length",
               source.length());

  SegmentedString excluded_line_number_source(source);
  excluded_line_number_source.SetExcludeLineNumbers();
  input_.InsertAtCurrentInsertionPoint(excluded_line_number_source);
  PumpTokenizerIfPossible();

  if (IsPaused()) {
    // Check the document.write() output with a separate preload scanner as
    // the main scanner can't deal with insertions.
    if (!insertion_preload_scanner_) {
      insertion_preload_scanner_ =
          CreatePreloadScanner(TokenPreloadScanner::ScannerType::kInsertion);
    }
    insertion_preload_scanner_->AppendToEnd(source);
    ScanAndPreload(insertion_preload_scanner_.get());
  }

  EndIfDelayed();
}

void HTMLDocumentParser::Append(const String& input_source) {
  if (IsStopped())
    return;

  TRACE_EVENT1(TRACE_DISABLED_BY_DEFAULT("blink.debug"),
               "HTMLDocumentParser::append", "size", input_source.length());
  const SegmentedString source(input_source);

  if (GetDocument()->IsPrefetchOnly()) {
    // Do not prefetch if there is an appcache.
    if (GetDocument()->Loader()->GetResponse().AppCacheID() != 0)
      return;

    if (!preload_scanner_) {
      preload_scanner_ =
          CreatePreloadScanner(TokenPreloadScanner::ScannerType::kMainDocument);
    }

    preload_scanner_->AppendToEnd(source);
    ScanAndPreload(preload_scanner_.get());

    // Return after the preload scanner, do not actually parse the document.
    return;
  }

  if (preload_scanner_) {
    if (input_.Current().IsEmpty() && !IsPaused()) {
      // We have parsed until the end of the current input and so are now moving
      // ahead of the preload scanner. Clear the scanner so we know to scan
      // starting from the current input point if we block again.
      preload_scanner_.reset();
    } else {
      preload_scanner_->AppendToEnd(source);
      if (IsPaused())
        ScanAndPreload(preload_scanner_.get());
    }
  }

  input_.AppendToEnd(source);

  if (InPumpSession()) {
    // We've gotten data off the network in a nested write. We don't want to
    // consume any more of the input stream now.  Do not worry.  We'll consume
    // this data in a less-nested write().
    return;
  }

  PumpTokenizerIfPossible();

  EndIfDelayed();
}

void HTMLDocumentParser::end() {
  DCHECK(!IsDetached());

  // Informs the the rest of WebCore that parsing is really finished (and
  // deletes this).
  tree_builder_->Finished();

  DocumentParser::StopParsing();
}

void HTMLDocumentParser::AttemptToRunDeferredScriptsAndEnd() {
  DCHECK(IsStopping());
  // That means hasInsertionPoint() may not be correct in some cases.
  DCHECK(!HasInsertionPoint());
  if (script_runner_ && !script_runner_->ExecuteScriptsWaitingForParsing())
    return;
  end();
}

void HTMLDocumentParser::AttemptToEnd() {
  // finish() indicates we will not receive any more data. If we are waiting on
  // an external script to load, we can't finish parsing quite yet.

  if (ShouldDelayEnd()) {
    end_was_delayed_ = true;
    return;
  }
  PrepareToStopParsing();
}

void HTMLDocumentParser::EndIfDelayed() {
  // If we've already been detached, don't bother ending.
  if (IsDetached())
    return;

  if (!end_was_delayed_ || ShouldDelayEnd())
    return;

  end_was_delayed_ = false;
  PrepareToStopParsing();
}

void HTMLDocumentParser::Finish() {
  // FIXME: We should DCHECK(!m_parserStopped) here, since it does not makes
  // sense to call any methods on DocumentParser once it's been stopped.
  // However, FrameLoader::stop calls DocumentParser::finish unconditionally.

  Flush();
  if (IsDetached())
    return;

  // We're not going to get any more data off the network, so we tell the input
  // stream we've reached the end of file. finish() can be called more than
  // once, if the first time does not call end().
  if (!input_.HaveSeenEndOfFile())
    input_.MarkEndOfFile();

  PumpTokenizerIfPossible();

  AttemptToEnd();
}

bool HTMLDocumentParser::IsExecutingScript() const {
  if (!script_runner_)
    return false;
  return script_runner_->IsExecutingScript();
}

OrdinalNumber HTMLDocumentParser::LineNumber() const {
  return input_.Current().CurrentLine();
}

TextPosition HTMLDocumentParser::GetTextPosition() const {
  const SegmentedString& current_string = input_.Current();
  OrdinalNumber line = current_string.CurrentLine();
  OrdinalNumber column = current_string.CurrentColumn();

  return TextPosition(line, column);
}

bool HTMLDocumentParser::IsWaitingForScripts() const {
  // When the TreeBuilder encounters a </script> tag, it returns to the
  // HTMLDocumentParser where the script is transfered from the treebuilder to
  // the script runner. The script runner will hold the script until its loaded
  // and run. During any of this time, we want to count ourselves as "waiting
  // for a script" and thus run the preload scanner, as well as delay completion
  // of parsing.
  bool tree_builder_has_blocking_script =
      tree_builder_->HasParserBlockingScript();
  bool script_runner_has_blocking_script =
      script_runner_ && script_runner_->HasParserBlockingScript();
  // Since the parser is paused while a script runner has a blocking script, it
  // should never be possible to end up with both objects holding a blocking
  // script.
  DCHECK(
      !(tree_builder_has_blocking_script && script_runner_has_blocking_script));
  // If either object has a blocking script, the parser should be paused.
  return tree_builder_has_blocking_script ||
         script_runner_has_blocking_script ||
         reentry_permit_->ParserPauseFlag();
}

void HTMLDocumentParser::ResumeParsingAfterPause() {
  DCHECK(!IsExecutingScript());
  DCHECK(!IsPaused());

  CheckIfBodyStylesheetAdded();
  if (IsPaused())
    return;

  insertion_preload_scanner_.reset();
  PumpTokenizerIfPossible();
  EndIfDelayed();
}

void HTMLDocumentParser::AppendCurrentInputStreamToPreloadScannerAndScan() {
  DCHECK(preload_scanner_);
  preload_scanner_->AppendToEnd(input_.Current());
  ScanAndPreload(preload_scanner_.get());
}

void HTMLDocumentParser::NotifyScriptLoaded(PendingScript* pending_script) {
  DCHECK(script_runner_);
  DCHECK(!IsExecutingScript());

  if (IsStopped()) {
    return;
  }

  if (IsStopping()) {
    AttemptToRunDeferredScriptsAndEnd();
    return;
  }

  script_runner_->ExecuteScriptsWaitingForLoad(pending_script);
  if (!IsPaused())
    ResumeParsingAfterPause();
}

void HTMLDocumentParser::ExecuteScriptsWaitingForResources() {
  if (IsStopped())
    return;

  DCHECK(GetDocument()->IsScriptExecutionReady());

  if (is_waiting_for_stylesheets_)
    is_waiting_for_stylesheets_ = false;

  // Document only calls this when the Document owns the DocumentParser so this
  // will not be called in the DocumentFragment case.
  DCHECK(script_runner_);
  script_runner_->ExecuteScriptsWaitingForResources();
  if (!IsPaused())
    ResumeParsingAfterPause();
}

void HTMLDocumentParser::DidAddPendingStylesheetInBody() {
  // When in-body CSS doesn't block painting, the parser needs to pause so that
  // the DOM doesn't include any elements that may depend on the CSS for style.
  // The stylesheet can be added and removed during the parsing of a single
  // token so don't actually set the bit to block parsing here, just track
  // the state of the added sheet in case it does persist beyond a single
  // token.
  if (RuntimeEnabledFeatures::CSSInBodyDoesNotBlockPaintEnabled())
    added_pending_stylesheet_in_body_ = true;
}

void HTMLDocumentParser::DidLoadAllBodyStylesheets() {
  // Just toggle the stylesheet flag here (mostly for synchronous sheets).
  // The document will also call into executeScriptsWaitingForResources
  // which is when the parser will re-start, otherwise it will attempt to
  // resume twice which could cause state machine issues.
  added_pending_stylesheet_in_body_ = false;
}

void HTMLDocumentParser::CheckIfBodyStylesheetAdded() {
  if (added_pending_stylesheet_in_body_) {
    added_pending_stylesheet_in_body_ = false;
    is_waiting_for_stylesheets_ = true;
  }
}

void HTMLDocumentParser::ParseDocumentFragment(
    const String& source,
    DocumentFragment* fragment,
    Element* context_element,
    ParserContentPolicy parser_content_policy) {
  HTMLDocumentParser* parser = HTMLDocumentParser::Create(
      fragment, context_element, parser_content_policy);
  parser->Append(source);
  parser->Finish();
  // Allows ~DocumentParser to assert it was detached before destruction.
  parser->Detach();
}

void HTMLDocumentParser::AppendBytes(const char* data, size_t length) {
  if (!length || IsStopped())
    return;

  EnsureTokenPreloadScanner();
  DecodedDataDocumentParser::AppendBytes(data, length);
}

void HTMLDocumentParser::Flush() {
  // If we've got no decoder, we never received any data.
  if (IsDetached() || NeedsDecoder())
    return;

  DecodedDataDocumentParser::Flush();
}

void HTMLDocumentParser::SetDecoder(
    std::unique_ptr<TextResourceDecoder> decoder) {
  DCHECK(decoder);
  DecodedDataDocumentParser::SetDecoder(std::move(decoder));
}

void HTMLDocumentParser::DocumentElementAvailable() {
  TRACE_EVENT0("blink,loader", "HTMLDocumentParser::documentElementAvailable");
  DCHECK(GetDocument()->documentElement());
  FetchQueuedPreloads();
}

std::unique_ptr<HTMLPreloadScanner> HTMLDocumentParser::CreatePreloadScanner(
    TokenPreloadScanner::ScannerType scanner_type) {
  return HTMLPreloadScanner::Create(
      options_, GetDocument()->Url(),
      CachedDocumentParameters::Create(GetDocument()),
      MediaValuesCached::MediaValuesCachedData(*GetDocument()), scanner_type);
}

void HTMLDocumentParser::ScanAndPreload(HTMLPreloadScanner* scanner) {
  PreloadRequestStream requests =
      scanner->Scan(GetDocument()->ValidBaseElementURL(), nullptr);
  preloader_->TakeAndPreload(requests);
}

void HTMLDocumentParser::FetchQueuedPreloads() {
  if (/*pending_csp_meta_token_ || */ !GetDocument()->documentElement())
    return;

  if (!queued_preloads_.IsEmpty())
    preloader_->TakeAndPreload(queued_preloads_);
}

}  // namespace blink
