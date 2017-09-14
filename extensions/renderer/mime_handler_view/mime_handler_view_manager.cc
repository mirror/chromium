// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/mime_handler_view/mime_handler_view_manager.h"

#include "base/lazy_instance.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/common/constants.h"
#include "extensions/renderer/mime_handler_view/mime_handler_view_frame_controller.h"
#include "ipc/ipc_message.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebFrame.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebRemoteFrame.h"

namespace extensions {

namespace {
using HandlerId = blink::WebMimeHandlerViewManager::HandlerId;

const char kMimeTypeApplicationPdf[] = "application/pdf";
const char kMimeTypeTextPdf[] = "text/pdf";

base::LazyInstance<
    std::unordered_map<blink::WebLocalFrame*, MimeHandlerViewManager*>>::
    DestructorAtExit g_web_local_frame_map_ = LAZY_INSTANCE_INITIALIZER;
}  // namespace

// static
MimeHandlerViewManager* MimeHandlerViewManager::Get(
    content::RenderFrame* render_frame) {
  blink::WebLocalFrame* owner_frame = render_frame->GetWebFrame();
  if (!g_web_local_frame_map_.Get().count(owner_frame))
    g_web_local_frame_map_.Get()[owner_frame] =
        new MimeHandlerViewManager(owner_frame);
  return g_web_local_frame_map_.Get()[owner_frame];
}

// static
void MimeHandlerViewManager::CreateInterface(
    int32_t owner_frame_routing_id,
    mime_handler_view::MimeHandlerViewManagerRequest request) {
  content::RenderFrame* render_frame =
      content::RenderFrame::FromRoutingID(owner_frame_routing_id);
  if (!render_frame)
    return;

  Get(render_frame)->binding_.Bind(std::move(request));
}

MimeHandlerViewManager::MimeHandlerViewManager(
    blink::WebLocalFrame* owner_frame)
    : blink::WebMimeHandlerViewManager(owner_frame), binding_(this) {}

MimeHandlerViewManager::~MimeHandlerViewManager() {
  g_web_local_frame_map_.Get().erase(owner_frame());
  binding_.Close();
}

HandlerId MimeHandlerViewManager::CreateHandler(
    const blink::WebURL& original_url,
    const blink::WebString& mime_type) {
  if (mime_type != kMimeTypeTextPdf && mime_type != kMimeTypeApplicationPdf)
    return blink::WebMimeHandlerViewManager::kHandlerIdNone;

  HandlerId id = NextHandlerId();
  frame_controller_map_[id].reset(
      new MimeHandlerViewFrameController(id, original_url, this));

  bool in_plugin_document = owner_frame()->GetDocument().IsPluginDocument();
  if (!in_plugin_document || id > kFirstHandlerId)
    frame_controller_map_[id]->RequestResource();

  return id;
}

blink::WebURL MimeHandlerViewManager::GetUrl(HandlerId id) const {
  if (!frame_controller_map_.count(id))
    return blink::WebURL();

  return GURL(base::StringPrintf(
      "%s://%s/index.html?%s", kExtensionScheme,
      extension_misc::kPdfExtensionId,
      frame_controller_map_.at(id)->resource_url().spec().c_str()));
}

v8::Local<v8::Object> MimeHandlerViewManager::V8ScriptableObject(
    HandlerId id,
    v8::Isolate* isolate) {
  DCHECK(frame_controller_map_.count(id));
  return frame_controller_map_[id]->V8ScriptableObject(isolate);
}

void MimeHandlerViewManager::DidReceiveDataInPluginDocument(
    const char* data,
    size_t data_length) {
  HandlerId id = kFirstHandlerId;
  DCHECK(frame_controller_map_.count(id));
  frame_controller_map_[id]->DidReceiveData(data, data_length);
  frame_controller_map_[id]->DidFinishLoading(0.0f);
}

void MimeHandlerViewManager::DidPauseNavigation(int32_t render_frame_routing_id,
                                                int32_t frame_tree_node_id) {
  content::RenderFrame* navigating_render_frame =
      content::RenderFrame::FromRoutingID(render_frame_routing_id);
  if (!navigating_render_frame)
    return;

  HandlerId id = blink::WebMimeHandlerViewManager::GetHandlerId(
      navigating_render_frame->GetWebFrame());

  DCHECK_NE(kHandlerIdNone, id);
  frame_controller_map_[id]->DidPauseNavigation(frame_tree_node_id);
}

void MimeHandlerViewManager::DocumentLoaded(int32_t frame_tree_node_id) {
  HandlerId id = GetHandlerIdFromFrameTreeNodeId(frame_tree_node_id);
  if (!frame_controller_map_.count(id))

    return;
  DCHECK(frame_controller_map_[id]);
  frame_controller_map_[id]->DocumentLoaded();
}

void MimeHandlerViewManager::FrameRemoved(int32_t frame_tree_node_id) {
  HandlerId id = GetHandlerIdFromFrameTreeNodeId(frame_tree_node_id);
  DCHECK_NE(kHandlerIdNone, id);
  RemoveFrameController(id);
}

void MimeHandlerViewManager::DidReceiveViewId(HandlerId id) {
  DCHECK(frame_controller_map_.count(id));
  if (auto* interface = GetBrowserInterface()) {
    interface->ReleaseStream(frame_controller_map_[id]->frame_tree_node_id(),
                             frame_controller_map_[id]->view_id());
  }
}

void MimeHandlerViewManager::DidFailLoadingResource(HandlerId id) {
  RemoveFrameController(id);
}

mime_handler_view::MimeHandlerViewManagerHost*
MimeHandlerViewManager::GetBrowserInterface() {
  if (!browser_interface_.get()) {
    content::RenderFrame::FromWebFrame(owner_frame())
        ->GetRemoteInterfaces()
        ->GetInterface(&browser_interface_);
  }
  return browser_interface_.get();
}

HandlerId MimeHandlerViewManager::GetHandlerIdFromFrameTreeNodeId(
    int32_t frame_tree_node_id) {
  for (const auto& pair : frame_controller_map_) {
    if (pair.second->frame_tree_node_id() == frame_tree_node_id)
      return pair.first;
  }
  return kHandlerIdNone;
}

void MimeHandlerViewManager::RemoveFrameController(HandlerId id) {
  UnassignHandlerId(id);
  frame_controller_map_.erase(id);
  if (!frame_controller_map_.size()) {
    // No need to leave the channel open.
    binding_.Close();
  }

  // TODO(ekaramad): Should we try to delete |this| when there are no more embed
  // elements in the frame.
}

}  // namespace extensions
