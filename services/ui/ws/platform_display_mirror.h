// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_PLATFORM_DISPLAY_MIRROR_H_
#define SERVICES_UI_WS_PLATFORM_DISPLAY_MIRROR_H_

#include <memory>

#include "base/macros.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/host/host_frame_sink_client.h"
#include "services/ui/ws/platform_display.h"
#include "ui/platform_window/platform_window.h"
#include "ui/platform_window/platform_window_delegate.h"
#include "ui/display/display.h"

namespace ui {
namespace ws {

class Display;
class WindowServer;

// PlatformDisplay implementation that mirrors another display.
class PlatformDisplayMirror : public PlatformDisplay,
                              public ui::PlatformWindowDelegate,
                              public viz::HostFrameSinkClient {
 public:
  PlatformDisplayMirror(const display::Display& display,
                        const display::ViewportMetrics& metrics,
                        // const viz::FrameSinkId& frame_sink_id,
                        WindowServer* window_server,
                        Display* display_to_mirror);
  ~PlatformDisplayMirror() override;

  const display::Display& display() const { return display_; }
  Display* display_to_mirror() const { return display_to_mirror_; }

  // PlatformDisplay:
  void Init(PlatformDisplayDelegate* delegate) override {}
  void SetViewportSize(const gfx::Size& size) override {}
  void SetTitle(const base::string16& title) override {}
  void SetCapture() override {}
  void ReleaseCapture() override {}
  void SetCursor(const ui::CursorData& cursor) override {}
  void MoveCursorTo(const gfx::Point& window_pixel_location) override {}
  void SetCursorSize(const ui::CursorSize& cursor_size) override {}
  void ConfineCursorToBounds(const gfx::Rect& pixel_bounds) override {}
  void UpdateTextInputState(const ui::TextInputState& state) override {}
  void SetImeVisibility(bool visible) override {}
  void UpdateViewportMetrics(
      const display::ViewportMetrics& metrics) override {}
  const display::ViewportMetrics& GetViewportMetrics() override;
  gfx::AcceleratedWidget GetAcceleratedWidget() const override;
  FrameGenerator* GetFrameGenerator() override;
  EventSink* GetEventSink() override;
  void SetCursorConfig(display::Display::Rotation rotation,
                       float scale) override {}

  // ui::PlatformWindowDelegate:
  void OnBoundsChanged(const gfx::Rect& new_bounds) override {}
  void OnDamageRect(const gfx::Rect& damaged_region) override {}
  void DispatchEvent(ui::Event* event) override {}
  void OnCloseRequest() override {}
  void OnClosed() override {}
  void OnWindowStateChanged(ui::PlatformWindowState new_state) override {}
  void OnLostCapture() override {}
  void OnAcceleratedWidgetAvailable(gfx::AcceleratedWidget widget,
                                    float device_scale_factor) override;
  void OnAcceleratedWidgetDestroyed() override {}
  void OnActivationChanged(bool active) override {}

  // viz::HostFrameSinkClient:
  void OnFirstSurfaceActivation(const viz::SurfaceInfo& surface_info) override {}
  void OnFrameTokenChanged(uint32_t frame_token) override {}

 private:
  display::Display display_;
  const display::ViewportMetrics& metrics_;
  WindowServer* window_server_;
  viz::FrameSinkId frame_sink_id_;
  Display* display_to_mirror_;
  std::unique_ptr<ui::PlatformWindow> platform_window_;
  gfx::AcceleratedWidget widget_ = gfx::kNullAcceleratedWidget;
  std::unique_ptr<FrameGenerator> frame_generator_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(PlatformDisplayMirror);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_PLATFORM_DISPLAY_MIRROR_H_
