// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/extensions_render_frame_observer.h"

#include <stddef.h>

#include "base/feature_list.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/content_features.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_messages.h"
#include "extensions/common/stack_frame.h"
#include "extensions/renderer/mime_handler_view/mime_handler_view_manager.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "third_party/WebKit/public/platform/WebURL.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/web/WebDataSource.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebView.h"

namespace extensions {

namespace {

// The delimiter for a stack trace provided by WebKit.
const char kStackFrameDelimiter[] = "\n    at ";

// Get a stack trace from a WebKit console message.
// There are three possible scenarios:
// 1. WebKit gives us a stack trace in |stack_trace|.
// 2. The stack trace is embedded in the error |message| by an internal
//    script. This will be more useful than |stack_trace|, since |stack_trace|
//    will include the internal bindings trace, instead of a developer's code.
// 3. No stack trace is included. In this case, we should mock one up from
//    the given line number and source.
// |message| will be populated with the error message only (i.e., will not
// include any stack trace).
StackTrace GetStackTraceFromMessage(base::string16* message,
                                    const base::string16& source,
                                    const base::string16& stack_trace,
                                    int32_t line_number) {
  StackTrace result;
  std::vector<base::string16> pieces;
  size_t index = 0;

  if (message->find(base::UTF8ToUTF16(kStackFrameDelimiter)) !=
          base::string16::npos) {
    pieces = base::SplitStringUsingSubstr(
        *message, base::UTF8ToUTF16(kStackFrameDelimiter),
        base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    *message = pieces[0];
    index = 1;
  } else if (!stack_trace.empty()) {
    pieces = base::SplitStringUsingSubstr(
        stack_trace, base::UTF8ToUTF16(kStackFrameDelimiter),
        base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  }

  // If we got a stack trace, parse each frame from the text.
  if (index < pieces.size()) {
    for (; index < pieces.size(); ++index) {
      std::unique_ptr<StackFrame> frame =
          StackFrame::CreateFromText(pieces[index]);
      if (frame.get())
        result.push_back(*frame);
    }
  }

  if (result.empty()) {  // If we don't have a stack trace, mock one up.
    result.push_back(
        StackFrame(line_number,
                   1u,  // column number
                   source,
                   base::string16() /* no function name */ ));
  }

  return result;
}

}  // namespace

ExtensionsRenderFrameObserver::ExtensionsRenderFrameObserver(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      webview_visually_deemphasized_(false) {
  render_frame->GetInterfaceRegistry()->AddInterface(
      base::Bind(&ExtensionsRenderFrameObserver::BindAppWindowRequest,
                 base::Unretained(this)));
}

ExtensionsRenderFrameObserver::~ExtensionsRenderFrameObserver() {
}

void ExtensionsRenderFrameObserver::BindAppWindowRequest(
    const service_manager::BindSourceInfo& source_info,
    mojom::AppWindowRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void ExtensionsRenderFrameObserver::SetVisuallyDeemphasized(bool deemphasized) {
  if (webview_visually_deemphasized_ == deemphasized)
    return;

  webview_visually_deemphasized_ = deemphasized;

  SkColor color =
      deemphasized ? SkColorSetARGB(178, 0, 0, 0) : SK_ColorTRANSPARENT;
  render_frame()->GetRenderView()->GetWebView()->SetPageOverlayColor(color);
}

void ExtensionsRenderFrameObserver::DetailedConsoleMessageAdded(
    const base::string16& message,
    const base::string16& source,
    const base::string16& stack_trace_string,
    uint32_t line_number,
    int32_t severity_level) {
  base::string16 trimmed_message = message;
  StackTrace stack_trace = GetStackTraceFromMessage(
      &trimmed_message,
      source,
      stack_trace_string,
      line_number);
  Send(new ExtensionHostMsg_DetailedConsoleMessageAdded(
      routing_id(), trimmed_message, source, stack_trace, severity_level));
}

void ExtensionsRenderFrameObserver::OnDestruct() {
  delete this;
}

void ExtensionsRenderFrameObserver::DidStartProvisionalLoad(
    blink::WebDataSource* data_source) {
  if (!base::FeatureList::IsEnabled(features::kWebAccessiblePdfExtension))
    return;

  GURL url(data_source->GetRequest().Url());

  if (url.scheme() != kExtensionScheme)
    return;

  if (url.host() != extension_misc::kPdfExtensionId)
    return;

  blink::WebFrame* parent = render_frame()->GetWebFrame()->Parent();
  if (!parent || parent->IsWebRemoteFrame()) {
    // The request for the resource is sent through the embedder process; hence
    // the parent cannot be remote. Furthermore, we do not navigate the main
    // frame to the extension process as the PDF is always added to the document
    // through an <embed> tag (which has a content frame).
    return;
  }

  MimeHandlerViewManager::FromRenderFrame(
      content::RenderFrame::FromWebFrame(parent->ToWebLocalFrame()))
      ->MaybeRequestResource(render_frame(),
                             GURL(data_source->GetRequest().Url()));
}

}  // namespace extensions
