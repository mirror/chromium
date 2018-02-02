// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/window_data.h"

#include "ui/aura/window.h"

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
  WindowData* data = new WindowData(client, window_id, frame_sink_id);
  window->SetProperty(kWindowDataKey, data);
  return data;
}

// static
const WindowData* WindowData::Get(const aura::Window* window) {
  return window->GetProperty(kWindowDataKey);
}

WindowData::WindowData(WindowServiceClient* client,
                       WindowId window_id,
                       const viz::FrameSinkId& frame_sink_id)
    : owning_window_service_client_(client),
      window_id_(window_id),
      frame_sink_id_(frame_sink_id) {}

}  // namespace ws2
}  // namespace ui
