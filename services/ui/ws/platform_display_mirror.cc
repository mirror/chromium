// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/platform_display_mirror.h"

#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "services/ui/ws/display.h"
#include "services/ui/ws/frame_generator.h"
#include "services/ui/ws/threaded_image_cursors.h"

namespace ui {
namespace ws {

PlatformDisplayMirror::PlatformDisplayMirror(
    ServerWindow* root_window,
    Display* display_to_mirror,
    const display::ViewportMetrics& metrics,
    std::unique_ptr<ThreadedImageCursors> image_cursors)
    : PlatformDisplayDefault(root_window, metrics, std::move(image_cursors)),
      display_to_mirror_(display_to_mirror) {
  DCHECK(display_to_mirror_);
}

PlatformDisplayMirror::~PlatformDisplayMirror() = default;

void PlatformDisplayMirror::OnAcceleratedWidgetAvailable(
    gfx::AcceleratedWidget widget,
    float device_scale_factor) {
  LOG(ERROR) << "MSW PlatformDisplayMirror::OnAcceleratedWidgetAvailable "; 
  PlatformDisplayDefault::OnAcceleratedWidgetAvailable(widget, device_scale_factor);
  // Pass the surface info for the mirror source display to the frame generator,
  // the id is not available if the source display init is not yet complete.
  const viz::SurfaceInfo& info = display_to_mirror_->platform_display()->GetFrameGenerator()->window_manager_surface_info();
  if (info.id().is_valid())
    GetFrameGenerator()->OnFirstSurfaceActivation(viz::SurfaceInfo(info.id(), metrics().device_scale_factor, metrics().bounds_in_pixels.size()));

}

}  // namespace ws
}  // namespace ui
