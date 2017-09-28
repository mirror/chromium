// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "public/web/WebMimeHandlerViewManager.h"
#include "core/frame/FrameOwner.h"
#include "core/frame/LocalFrameClient.h"
#include "core/html/HTMLFrameOwnerElement.h"
#include "core/html/HTMLPlugInElement.h"
#include "public/web/WebFrame.h"
#include "public/web/WebLocalFrame.h"

namespace blink {
namespace {
using HandlerId = WebMimeHandlerViewManager::HandlerId;

HTMLPlugInElement* GetHTMLPlugInElementFromContentFrame(
    const WebFrame* web_frame) {
  if (!web_frame)
    return nullptr;

  FrameOwner* owner = WebFrame::ToCoreFrame(*web_frame)->Owner();
  if (!owner)
    return nullptr;

  HTMLFrameOwnerElement* html_owner = ToHTMLFrameOwnerElement(owner);
  if (!IsHTMLPlugInElement(html_owner))
    return nullptr;

  return ToHTMLPlugInElement(html_owner);
}

}  // namespace

// static
const HandlerId WebMimeHandlerViewManager::kHandlerIdNone = -1;

// static
const HandlerId WebMimeHandlerViewManager::kFirstHandlerId = 0;

// static
HandlerId WebMimeHandlerViewManager::GetHandlerId(const WebFrame* web_frame) {
  if (auto* html_plugin_element =
          GetHTMLPlugInElementFromContentFrame(web_frame)) {
    return html_plugin_element->external_handler_id();
  }
  return kHandlerIdNone;
}

WebMimeHandlerViewManager::WebMimeHandlerViewManager(WebLocalFrame* owner_frame)
    : owner_frame_(owner_frame) {}
WebMimeHandlerViewManager::~WebMimeHandlerViewManager() {}

HandlerId WebMimeHandlerViewManager::NextHandlerId() {
  return next_handler_id_++;
}

WebFrame* WebMimeHandlerViewManager::GetContentFrameFromId(HandlerId id) {
  if (id > next_handler_id_)
    return nullptr;

  WebFrame* child = owner_frame_->FirstChild();
  while (child) {
    auto* html_plugin_element = GetHTMLPlugInElementFromContentFrame(child);
    if (html_plugin_element && id == html_plugin_element->external_handler_id())
      return child;
    child = child->NextSibling();
  }
  return nullptr;
}

void WebMimeHandlerViewManager::UnassignHandlerId(HandlerId id) {
  if (id > next_handler_id_ || id == kHandlerIdNone)
    return;

  WebFrame* child = owner_frame_->FirstChild();
  while (child) {
    auto* html_plugin_element = GetHTMLPlugInElementFromContentFrame(child);
    if (html_plugin_element && html_plugin_element->external_handler_id() == id)
      return html_plugin_element->ResetExternalHandlerId();
    child = child->NextSibling();
  }
}

}  // namespace blink
