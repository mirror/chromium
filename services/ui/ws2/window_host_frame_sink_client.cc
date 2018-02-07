// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/window_host_frame_sink_client.h"

#include "base/logging.h"
#include "services/ui/ws2/window_data.h"
#include "ui/aura/mus/client_surface_embedder.h"

namespace ui {
namespace ws2 {

WindowHostFrameSinkClient::WindowHostFrameSinkClient(aura::Window* window)
    : window_(window) {
  // TODO: args are not correct.
  // client_surface_embedder_ =
  // std::make_unique<aura::ClientSurfaceEmbedder>(window, false,
  // gfx::Insets());
  UpdateClientSurfaceEmbedder();
  LOG(ERROR) << window_;
}

WindowHostFrameSinkClient::~WindowHostFrameSinkClient() {}

void WindowHostFrameSinkClient::OnFrameSinkIdChanged() {
  UpdateClientSurfaceEmbedder();
}

void WindowHostFrameSinkClient::OnFirstSurfaceActivation(
    const viz::SurfaceInfo& surface_info) {
  /*
  viz::HostFrameSinkManager* host_frame_sink_manager =
      aura::Env::GetInstance()->context_factory_private()->GetHostFrameSinkManager();
    host_frame_sink_manager_->AssignTemporaryReference(
        surface_id, current->frame_sink_id());
  */
  LOG(ERROR) << "OH NOES";
}

void WindowHostFrameSinkClient::OnFrameTokenChanged(uint32_t frame_token) {
  NOTIMPLEMENTED();
}

void WindowHostFrameSinkClient::UpdateClientSurfaceEmbedder() {
  /*
  WindowData* window_data = WindowData::Get(window_);
  DCHECK(window_data);
  // TODO: only do this when size changes.
  local_surface_id_ = parent_local_surface_id_allocator_.GenerateId();
  window_data->frame_sink_id();
  const viz::SurfaceId surface_id =
      viz::SurfaceId(window_data->frame_sink_id(), local_surface_id_);
  client_surface_embedder_->SetPrimarySurfaceId(surface_id);
  */
}

}  // namespace ws2
}  // namespace ui
