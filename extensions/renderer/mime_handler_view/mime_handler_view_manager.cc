// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/mime_handler_view/mime_handler_view_manager.h"

#include "base/lazy_instance.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/renderer/mime_handler_view/mime_handler_view_frame_controller.h"

namespace extensions {

namespace {
base::LazyInstance<std::unordered_map<int32_t, MimeHandlerViewManager*>>::
    DestructorAtExit g_render_frame_map = LAZY_INSTANCE_INITIALIZER;
}

// static
MimeHandlerViewManager* MimeHandlerViewManager::Get(
    content::RenderFrame* render_frame) {
  int32_t routing_id = render_frame->GetRoutingID();
  if (!g_render_frame_map.Get().count(routing_id))
    g_render_frame_map.Get()[routing_id] =
        new MimeHandlerViewManager(render_frame);
  return g_render_frame_map.Get()[routing_id];
}

// static
void MimeHandlerViewManager::CreateInterface(
    int32_t owner_frame_routing_id,
    const service_manager::BindSourceInfo& source_info,
    mime_handler_view::MimeHandlerViewManagerRequest request) {
  content::RenderFrame* render_frame =
      content::RenderFrame::FromRoutingID(owner_frame_routing_id);
  if (!render_frame)
    return;

  Get(render_frame)->binding_.Bind(std::move(request));
}

MimeHandlerViewManager::MimeHandlerViewManager(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      internal_map_key_(render_frame->GetRoutingID()),
      binding_(this) {}

MimeHandlerViewManager::~MimeHandlerViewManager() {
  g_render_frame_map.Get().erase(internal_map_key_);
}

void MimeHandlerViewManager::RequestResource(int32_t frame_controller_id,
                                             int32_t routing_id,
                                             const std::string& resource_url) {
  if (pending_frame_controller_map_.count(routing_id)) {
    frame_controller_map_[frame_controller_id] =
        std::move(pending_frame_controller_map_[routing_id]);
    pending_frame_controller_map_.erase(routing_id);
  }

  if (!frame_controller_map_.count(frame_controller_id)) {
    // Depending on whether or not there have been an scripting access to
    // <embed> object we might or might not have created the frame controller.
    frame_controller_map_[frame_controller_id].reset(
        new MimeHandlerViewFrameController(this));
  }
  frame_controller_map_[frame_controller_id]->SetId(frame_controller_id);
  frame_controller_map_[frame_controller_id]->RequestResource(resource_url);
}

void MimeHandlerViewManager::DocumentLoaded(int32_t frame_controller_id) {
  if (!frame_controller_map_.count(frame_controller_id))
    return;
  DCHECK(frame_controller_map_[frame_controller_id]);
  frame_controller_map_[frame_controller_id]->DocumentLoaded();
}

void MimeHandlerViewManager::RemoveFrameController(
    int32_t frame_controller_id) {
  frame_controller_map_.erase(frame_controller_id);
  if (!frame_controller_map_.size() && !pending_frame_controller_map_.size())
    delete this;
}

void MimeHandlerViewManager::Destroy() {
  delete this;
}

void MimeHandlerViewManager::OnDestruct() {
  delete this;
}

void MimeHandlerViewManager::SetViewId(int32_t frame_controller_id,
                                       const std::string& view_id) {
  DCHECK(frame_controller_map_.count(frame_controller_id));
  if (auto* interface = GetBrowserInterface())
    interface->ReleaseStream(frame_controller_id, view_id);
}

mime_handler_view::MimeHandlerViewManagerHost*
MimeHandlerViewManager::GetBrowserInterface() {
  if (!browser_interface_.get()) {
    render_frame()->GetRemoteInterfaces()->GetInterface(&browser_interface_);
  }
  return browser_interface_.get();
}

}  // namespace extensions
