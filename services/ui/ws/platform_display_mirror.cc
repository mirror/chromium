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
#include "ui/platform_window/platform_window.h"

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#endif

namespace ui {
namespace ws {

PlatformDisplayMirror::PlatformDisplayMirror(
    const display::Display& display,
    const display::ViewportMetrics& metrics,
    WindowServer* window_server,
    // const viz::FrameSinkId& frame_sink_id,
    Display* display_to_mirror)
    : display_(display),
      metrics_(metrics),
      window_server_(window_server),
      // frame_sink_id_(frame_sink_id),
      display_to_mirror_(display_to_mirror) {
  DCHECK(display_to_mirror_);

  WindowId id = window_server->display_manager()->GetAndAdvanceNextRootId();
  frame_sink_id_ = viz::FrameSinkId(id.client_id, id.window_id);

  // Create a new platform window to display the mirror destination content.
  const gfx::Rect& bounds = metrics_.bounds_in_pixels;
  DCHECK(!bounds.size().IsEmpty());

#if defined(OS_WIN)
  platform_window_ = std::make_unique<ui::WinWindow>(this, bounds);
#elif defined(USE_X11)
  platform_window_ = std::make_unique<ui::X11Window>(this, bounds);
#elif defined(OS_ANDROID)
  platform_window_ = std::make_unique<ui::PlatformWindowAndroid>(this);
  platform_window_->SetBounds(bounds);
#elif defined(USE_OZONE)
  platform_window_ =
      OzonePlatform::GetInstance()->CreatePlatformWindow(this, bounds);
#else
  NOTREACHED() << "Unsupported platform";
#endif

  platform_window_->Show();
}

PlatformDisplayMirror::~PlatformDisplayMirror() = default;

const display::ViewportMetrics& PlatformDisplayMirror::GetViewportMetrics() { return metrics_; }
gfx::AcceleratedWidget PlatformDisplayMirror::GetAcceleratedWidget() const { return widget_; }
FrameGenerator* PlatformDisplayMirror::GetFrameGenerator() { return frame_generator_.get(); }
EventSink* PlatformDisplayMirror::GetEventSink() { return nullptr; }

void PlatformDisplayMirror::OnAcceleratedWidgetAvailable(
    gfx::AcceleratedWidget widget,
    float device_scale_factor) {
  LOG(ERROR) << "MSW PlatformDisplayMirror::OnAcceleratedWidgetAvailable A"; 

  // TODO(msw): Init or ctor? 
  // This will get called after Init() is called, either synchronously as part
  // of the Init() callstack or async after Init() has returned, depending on
  // the platform.
  DCHECK_EQ(gfx::kNullAcceleratedWidget, widget_);
  widget_ = widget;

  // delegate_->OnAcceleratedWidgetAvailable(); {
    // display_manager()->OnDisplayAcceleratedWidgetAvailable(this); {
      // window_server_->OnDisplayReady(display, is_first_display);
    // InitWindowManagerDisplayRoots();
  // } // delegate_->OnAcceleratedWidgetAvailable();

  // if (!delegate_->IsHostingViz())
  //   return;

  // Access CreateRootCompositorFrameSink()...
  viz::mojom::CompositorFrameSinkAssociatedPtr compositor_frame_sink;
  viz::mojom::DisplayPrivateAssociatedPtr display_private;
  viz::mojom::CompositorFrameSinkClientPtr compositor_frame_sink_client;
  viz::mojom::CompositorFrameSinkClientRequest
      compositor_frame_sink_client_request =
          mojo::MakeRequest(&compositor_frame_sink_client);

  LOG(ERROR) << "MSW PlatformDisplayMirror::OnAcceleratedWidgetAvailable B1"; 
  window_server_->GetVizHostProxy()->RegisterFrameSinkId(frame_sink_id_, this);
  LOG(ERROR) << "MSW PlatformDisplayMirror::OnAcceleratedWidgetAvailable B1"; 
  window_server_->GetVizHostProxy()->CreateRootCompositorFrameSink(
      frame_sink_id_, widget_,
      viz::CreateRendererSettings(viz::BufferToTextureTargetMap()),
      mojo::MakeRequest(&compositor_frame_sink),
      std::move(compositor_frame_sink_client),
      mojo::MakeRequest(&display_private));

  LOG(ERROR) << "MSW PlatformDisplayMirror::OnAcceleratedWidgetAvailable C"; 
  // Make a FrameGenerator that references the display_to_mirror's surface id? 
  display_private->SetDisplayVisible(true);
  frame_generator_ = std::make_unique<FrameGenerator>();
  auto frame_sink_client_binding =
      std::make_unique<CompositorFrameSinkClientBinding>(
          frame_generator_.get(),
          std::move(compositor_frame_sink_client_request),
          std::move(compositor_frame_sink), std::move(display_private));
  frame_generator_->Bind(std::move(frame_sink_client_binding));
  frame_generator_->OnWindowSizeChanged(metrics_.bounds_in_pixels.size()); // TODO(msw): Use the source display's ServerWindow size? 
  frame_generator_->SetDeviceScaleFactor(metrics_.device_scale_factor);

  LOG(ERROR) << "MSW PlatformDisplayMirror::OnAcceleratedWidgetAvailable D"; 
  // Pass the surface info for the mirror source display to the frame generator,
  // the id is not available if the source display init is not yet complete.
  const viz::SurfaceInfo& info = display_to_mirror_->platform_display()
                                     ->GetFrameGenerator()
                                     ->window_manager_surface_info();
  if (info.id().is_valid()) {
    LOG(ERROR) << "MSW PlatformDisplayMirror::OnAcceleratedWidgetAvailable E"; 
    GetFrameGenerator()->OnFirstSurfaceActivation(
        viz::SurfaceInfo(info.id(), metrics_.device_scale_factor,
                         metrics_.bounds_in_pixels.size()));
  }
}

}  // namespace ws
}  // namespace ui
