// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/history_serialization.h"

#include <stddef.h>
#include <third_party/WebKit/Source/platform/wtf/text/WTFString.h>

#include "base/strings/nullable_string16.h"
#include "content/child/web_url_request_util.h"
#include "content/common/page_state_serialization.h"
#include "content/public/common/page_state.h"
#include "content/renderer/history_entry.h"
#include "third_party/WebKit/public/platform/WebData.h"
#include "third_party/WebKit/public/platform/WebFloatPoint.h"
#include "third_party/WebKit/public/platform/WebHTTPBody.h"
#include "third_party/WebKit/public/platform/WebPoint.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebVector.h"
#include "third_party/WebKit/public/web/WebHistoryItem.h"
#include "third_party/WebKit/public/web/WebSerializedScriptValue.h"

using blink::WebData;
using blink::WebHTTPBody;
using blink::WebHistoryItem;
using blink::WebSerializedScriptValue;
using blink::WebString;
using blink::WebVector;

namespace content {
namespace {

void ToOptionalString16Vector(const WebVector<WebString>& input,
                              std::vector<base::Optional<base::string16>>* output) {
  output->reserve(output->size() + input.size());
  for (size_t i = 0; i < input.size(); ++i)
    output->push_back(input[i].Utf16());
}

WebString OptionalString16ToWebString(base::Optional<base::string16> str) {
  if(!str.has_value() || str.value().empty()) {
    return WebString();
  } else {
    return WebString::FromUTF16(str.value());
  }
}

void GenerateFrameStateFromItem(const WebHistoryItem& item,
                                ExplodedFrameState* state) {
  state->url_string = item.UrlString().Utf16();
  state->referrer = item.GetReferrer().Utf16();
  state->referrer_policy = item.GetReferrerPolicy();
  state->target = item.Target().Utf16();
  if (!item.StateObject().IsNull()) {
    state->state_object = item.StateObject().ToString().Utf16();
  }
  state->scroll_restoration_type = item.ScrollRestorationType();
  state->visual_viewport_scroll_offset = item.VisualViewportScrollOffset();
  state->scroll_offset = item.GetScrollOffset();
  state->item_sequence_number = item.ItemSequenceNumber();
  state->document_sequence_number = item.DocumentSequenceNumber();
  state->page_scale_factor = item.PageScaleFactor();
  state->did_save_scroll_or_scale_state = item.DidSaveScrollOrScaleState();
  ToOptionalString16Vector(item.GetDocumentState(), &state->document_state);

  state->http_body.http_content_type = item.HttpContentType().Utf16();
  const WebHTTPBody& http_body = item.HttpBody();
  if (!http_body.IsNull()) {
    state->http_body.request_body = GetRequestBodyForWebHTTPBody(http_body);
    state->http_body.contains_passwords = http_body.ContainsPasswordData();
  }
}

void RecursivelyGenerateFrameState(
    HistoryEntry::HistoryNode* node,
    ExplodedFrameState* state,
    std::vector<base::Optional<base::string16>>* referenced_files) {
  GenerateFrameStateFromItem(node->item(), state);
  ToOptionalString16Vector(node->item().GetReferencedFilePaths(),
                           referenced_files);

  std::vector<HistoryEntry::HistoryNode*> children = node->children();
  state->children.resize(children.size());
  for (size_t i = 0; i < children.size(); ++i) {
    RecursivelyGenerateFrameState(children[i], &state->children[i],
                                  referenced_files);
  }
}

void RecursivelyGenerateHistoryItem(const ExplodedFrameState& state,
                                    HistoryEntry::HistoryNode* node) {
  WebHistoryItem item;
  item.Initialize();
  item.SetURLString(OptionalString16ToWebString(state.url_string));
  item.SetReferrer(OptionalString16ToWebString(state.referrer), state.referrer_policy);
  item.SetTarget(OptionalString16ToWebString(state.target));
  if (state.state_object.has_value()) {
    item.SetStateObject(WebSerializedScriptValue::FromString(
        WebString::FromUTF16(state.state_object.value())));
  }
  WebVector<WebString> document_state(state.document_state.size());
  std::transform(state.document_state.begin(), state.document_state.end(),
                 document_state.begin(), [](const base::Optional<base::string16>& s) {
                   return OptionalString16ToWebString(s);
                 });
  item.SetDocumentState(document_state);
  item.SetScrollRestorationType(state.scroll_restoration_type);

  if (state.did_save_scroll_or_scale_state) {
    item.SetVisualViewportScrollOffset(state.visual_viewport_scroll_offset);
    item.SetScrollOffset(state.scroll_offset);
    item.SetPageScaleFactor(state.page_scale_factor);
  }

  // These values are generated at WebHistoryItem construction time, and we
  // only want to override those new values with old values if the old values
  // are defined.  A value of 0 means undefined in this context.
  if (state.item_sequence_number)
    item.SetItemSequenceNumber(state.item_sequence_number);
  if (state.document_sequence_number)
    item.SetDocumentSequenceNumber(state.document_sequence_number);

  item.SetHTTPContentType(
      OptionalString16ToWebString(state.http_body.http_content_type));
  if (state.http_body.request_body != nullptr) {
    item.SetHTTPBody(
        GetWebHTTPBodyForRequestBody(state.http_body.request_body));
  }
  node->set_item(item);

  for (size_t i = 0; i < state.children.size(); ++i)
    RecursivelyGenerateHistoryItem(state.children[i], node->AddChild());
}

}  // namespace

PageState HistoryEntryToPageState(HistoryEntry* entry) {
  ExplodedPageState state;
  RecursivelyGenerateFrameState(entry->root_history_node(), &state.top,
                                &state.referenced_files);

  std::string encoded_data;
  EncodePageState(state, &encoded_data);
  return PageState::CreateFromEncodedData(encoded_data);
}

PageState SingleHistoryItemToPageState(const WebHistoryItem& item) {
  ExplodedPageState state;
  ToOptionalString16Vector(item.GetReferencedFilePaths(),
                           &state.referenced_files);
  GenerateFrameStateFromItem(item, &state.top);

  std::string encoded_data;
  EncodePageState(state, &encoded_data);
  return PageState::CreateFromEncodedData(encoded_data);
}

std::unique_ptr<HistoryEntry> PageStateToHistoryEntry(
    const PageState& page_state) {
  ExplodedPageState state;
  if (!DecodePageState(page_state.ToEncodedData(), &state))
    return std::unique_ptr<HistoryEntry>();

  std::unique_ptr<HistoryEntry> entry(new HistoryEntry());
  RecursivelyGenerateHistoryItem(state.top, entry->root_history_node());

  return entry;
}

}  // namespace content
