// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/platform_display_mirror.h"

#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "components/viz/host/renderer_settings_creation.h"
#include "services/ui/ws/display.h"
#include "services/ui/ws/display_manager.h"
#include "services/ui/ws/frame_generator.h"
#include "services/ui/ws/window_server.h"
#include "ui/gfx/geometry/size_f.h"
#include "ui/platform_window/platform_window.h"

namespace ui {
namespace ws {

PlatformDisplayMirror::PlatformDisplayMirror(
    const display::Display& display,
    const display::ViewportMetrics& metrics,
    WindowServer* window_server,
    Display* display_to_mirror)
    : display_(display),
      metrics_(metrics),
      window_server_(window_server),
      display_to_mirror_(display_to_mirror) {
  DCHECK(display_to_mirror_);

  WindowId id = window_server->display_manager()->GetAndAdvanceNextRootId();
  frame_sink_id_ = viz::FrameSinkId(id.client_id, id.window_id);

  // Create a new platform window to display the mirror destination content.
  platform_window_ = CreatePlatformWindow(this, metrics_.bounds_in_pixels);
  platform_window_->Show();
}

PlatformDisplayMirror::~PlatformDisplayMirror() = default;

void PlatformDisplayMirror::UpdateViewportMetrics(
    const display::ViewportMetrics& metrics) {
  metrics_ = metrics;
}

const display::ViewportMetrics& PlatformDisplayMirror::GetViewportMetrics() {
  return metrics_;
}

gfx::AcceleratedWidget PlatformDisplayMirror::GetAcceleratedWidget() const {
  return widget_;
}

FrameGenerator* PlatformDisplayMirror::GetFrameGenerator() {
  return frame_generator_.get();
}

EventSink* PlatformDisplayMirror::GetEventSink() {
  return nullptr;
}

void PlatformDisplayMirror::OnAcceleratedWidgetAvailable(
    gfx::AcceleratedWidget widget,
    float device_scale_factor) {
  DCHECK_EQ(gfx::kNullAcceleratedWidget, widget_);
  widget_ = widget;

  // Create a CompositorFrameSink for this display, using the widget's surface.
  viz::mojom::CompositorFrameSinkAssociatedPtr compositor_frame_sink;
  viz::mojom::DisplayPrivateAssociatedPtr display_private;
  viz::mojom::CompositorFrameSinkClientPtr compositor_frame_sink_client;
  viz::mojom::CompositorFrameSinkClientRequest
      compositor_frame_sink_client_request =
          mojo::MakeRequest(&compositor_frame_sink_client);
  window_server_->GetVizHostProxy()->RegisterFrameSinkId(frame_sink_id_, this);
  window_server_->GetVizHostProxy()->CreateRootCompositorFrameSink(
      frame_sink_id_, widget_,
      viz::CreateRendererSettings(viz::BufferToTextureTargetMap()),
      mojo::MakeRequest(&compositor_frame_sink),
      std::move(compositor_frame_sink_client),
      mojo::MakeRequest(&display_private));

  // Make a FrameGenerator that references |display_to_mirror_|'s surface id.
  display_private->SetDisplayVisible(true);
  frame_generator_ = std::make_unique<FrameGenerator>();
  auto frame_sink_client_binding =
      std::make_unique<CompositorFrameSinkClientBinding>(
          frame_generator_.get(),
          std::move(compositor_frame_sink_client_request),
          std::move(compositor_frame_sink), std::move(display_private));
  frame_generator_->Bind(std::move(frame_sink_client_binding));

  // Report sizing and determine a scale factor for the mirroring destination.
  frame_generator_->OnWindowSizeChanged(metrics_.bounds_in_pixels.size());
  const gfx::SizeF source(display_to_mirror_->root_window()->bounds().size());
  const gfx::SizeF size(metrics_.bounds_in_pixels.size());
  frame_generator_->SetDeviceScaleFactor(
      std::min(size.width() / source.width(), size.height() / source.height()));

  // Pass the surface info for the mirror source display to the frame generator,
  // the id is not available if the source display init is not yet complete.
  const viz::SurfaceInfo& info = display_to_mirror_->platform_display()
                                     ->GetFrameGenerator()
                                     ->window_manager_surface_info();
  if (info.id().is_valid()) {
    frame_generator_->SetEmbeddedSurface(
        viz::SurfaceInfo(info.id(), metrics_.device_scale_factor,
                         metrics_.bounds_in_pixels.size()));
  }
}

}  // namespace ws
}  // namespace ui
