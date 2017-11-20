// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/platform_display_mirror.h"

#include "components/viz/common/surfaces/surface_info.h"
#include "services/ui/ws/frame_generator.h"
#include "services/ui/ws/threaded_image_cursors.h"

namespace ui {
namespace ws {

PlatformDisplayMirror::PlatformDisplayMirror(
    ServerWindow* root_window,
    const display::ViewportMetrics& metrics,
    std::unique_ptr<ThreadedImageCursors> image_cursors,
    const viz::SurfaceId& mirror_source_surface_id)
    : PlatformDisplayDefault(root_window, metrics, std::move(image_cursors)),
      mirror_source_surface_id_(mirror_source_surface_id) {}

PlatformDisplayMirror::~PlatformDisplayMirror() = default;

void PlatformDisplayMirror::OnAcceleratedWidgetAvailable(
    gfx::AcceleratedWidget widget,
    float device_scale_factor) {
  PlatformDisplayDefault::OnAcceleratedWidgetAvailable(widget, device_scale_factor);
  // Pass the surface info for the mirror source display to the frame generator.
  GetFrameGenerator()->OnFirstSurfaceActivation(viz::SurfaceInfo(mirror_source_surface_id_, metrics().device_scale_factor, metrics().bounds_in_pixels.size()));
}

}  // namespace ws
}  // namespace ui
