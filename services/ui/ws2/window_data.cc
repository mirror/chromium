// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/window_data.h"

#include "components/viz/host/host_frame_sink_manager.h"
#include "services/ui/ws2/window_host_frame_sink_client.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/compositor/compositor.h"

DEFINE_UI_CLASS_PROPERTY_TYPE(ui::ws2::WindowData*);

namespace ui {
namespace ws2 {
namespace {
DEFINE_OWNED_UI_CLASS_PROPERTY_KEY(ui::ws2::WindowData,
                                   kWindowDataKey,
                                   nullptr);
}  // namespace

WindowData::~WindowData() = default;

// static
WindowData* WindowData::Create(aura::Window* window,
                               WindowServiceClient* client,
                               WindowId window_id,
                               const viz::FrameSinkId& frame_sink_id) {
  DCHECK(!Get(window));
  // Owned by |window|.
  WindowData* data = new WindowData(window, client, window_id, frame_sink_id);
  window->SetProperty(kWindowDataKey, data);
  return data;
}

// static
const WindowData* WindowData::Get(const aura::Window* window) {
  return window->GetProperty(kWindowDataKey);
}

void WindowData::SetFrameSinkId(const viz::FrameSinkId& frame_sink_id) {
  UpdateFrameSinkIdInFrameSinkManager(frame_sink_id);
  frame_sink_id_ = frame_sink_id;
}

void WindowData::AttachCompositorFrameSink(
    viz::mojom::CompositorFrameSinkRequest compositor_frame_sink,
    viz::mojom::CompositorFrameSinkClientPtr client) {
  LOG(ERROR) << "AttachCompositorFrameSink id=" << frame_sink_id_.ToString();
  viz::HostFrameSinkManager* host_frame_sink_manager =
      aura::Env::GetInstance()
          ->context_factory_private()
          ->GetHostFrameSinkManager();
  if (!window_host_frame_sink_client_) {
    window_host_frame_sink_client_ =
        std::make_unique<WindowHostFrameSinkClient>(window_);
    host_frame_sink_manager->RegisterFrameSinkId(
        frame_sink_id_, window_host_frame_sink_client_.get());
    // XXX: this isn't quite right, and it needs to unregister!
    window_->layer()->GetCompositor()->AddFrameSink(frame_sink_id_);
    // TODO: figure out if need to register completely hierarchy.
  }
  host_frame_sink_manager->CreateCompositorFrameSink(
      frame_sink_id_, std::move(compositor_frame_sink), std::move(client));
}

WindowData::WindowData(aura::Window* window,
                       WindowServiceClient* client,
                       WindowId window_id,
                       const viz::FrameSinkId& frame_sink_id)
    : window_(window),
      owning_window_service_client_(client),
      window_id_(window_id),
      frame_sink_id_(frame_sink_id) {}

void WindowData::UpdateFrameSinkIdInFrameSinkManager(
    const viz::FrameSinkId& frame_sink_id) {
  if (!window_host_frame_sink_client_)
    return;

  LOG(ERROR) << "UpdateFrameSinkIdInFrameSinkManager id="
             << frame_sink_id_.ToString()
             << " new=" << frame_sink_id.ToString();
  // TODO: should this recurse?
  viz::HostFrameSinkManager* host_frame_sink_manager =
      aura::Env::GetInstance()
          ->context_factory_private()
          ->GetHostFrameSinkManager();
  host_frame_sink_manager->InvalidateFrameSinkId(frame_sink_id_);
  host_frame_sink_manager->RegisterFrameSinkId(
      frame_sink_id, window_host_frame_sink_client_.get());
  window_host_frame_sink_client_->OnFrameSinkIdChanged();
}

}  // namespace ws2
}  // namespace ui
