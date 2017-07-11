// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/renderer/mime_handler_view/mime_handler_view_manager.h"

#include "base/lazy_instance.h"
#include "content/public/renderer/render_frame.h"
#include "extensions/renderer/mime_handler_view/mime_handler_view_frame_controller.h"

namespace extensions {

namespace {
base::LazyInstance<
    std::unordered_map<content::RenderFrame*, MimeHandlerViewManager*>>::
    DestructorAtExit g_render_frame_map = LAZY_INSTANCE_INITIALIZER;
}

// static
MimeHandlerViewManager* MimeHandlerViewManager::Get(
    content::RenderFrame* render_frame) {
  if (!g_render_frame_map.Get().count(render_frame))
    g_render_frame_map.Get()[render_frame] =
        new MimeHandlerViewManager(render_frame);
  return g_render_frame_map.Get()[render_frame];
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

  Get(render_frame)->CreateInterfaceInternal(std::move(request));
}

MimeHandlerViewManager::MimeHandlerViewManager(
    content::RenderFrame* render_frame)
    : render_frame_(render_frame), binding_(this) {}

MimeHandlerViewManager::~MimeHandlerViewManager() {
  g_render_frame_map.Get().erase(render_frame_);
}

void MimeHandlerViewManager::RequestResource(int32_t frame_controller_id,
                                             const std::string& resource_url) {
  if (!frame_controller_map_.count(frame_controller_id)) {
    // Depending on whether or not there have been an scripting access to
    // <embed> object we might or might not have created the frame controller.
    frame_controller_map_[frame_controller_id].reset(
        new MimeHandlerViewFrameController(frame_controller_id));
  }
  frame_controller_map_[frame_controller_id]->RequestResource(resource_url);
}

void MimeHandlerViewManager::DocumentLoaded(int32_t frame_controller_id) {
  if (!frame_controller_map_.count(frame_controller_id))
    return;
  DCHECK(frame_controller_map_[frame_controller_id]);
  frame_controller_map_[frame_controller_id]->DocumentLoaded();
}

void MimeHandlerViewManager::FrameDestroyed(int32_t frame_controller_id) {
  frame_controller_map_.erase(frame_controller_id);
}

void MimeHandlerViewManager::CreateInterfaceInternal(
    mime_handler_view::MimeHandlerViewManagerRequest request) {
  binding_.Bind(std::move(request));
}

}  // namespace extensions
