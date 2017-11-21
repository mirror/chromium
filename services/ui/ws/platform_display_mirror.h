// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_PLATFORM_DISPLAY_MIRROR_H_
#define SERVICES_UI_WS_PLATFORM_DISPLAY_MIRROR_H_

#include <memory>

#include "base/macros.h"
#include "services/ui/ws/platform_display_default.h"

namespace ui {
namespace ws {

class Display;

// PlatformDisplay implementation that mirrors another display.
class PlatformDisplayMirror : public PlatformDisplayDefault {
 public:
  // |image_cursors| may be null, for example on Android or in tests.
  PlatformDisplayMirror(ServerWindow* root_window,
                        Display* display_to_mirror,
                        const display::ViewportMetrics& metrics,
                        std::unique_ptr<ThreadedImageCursors> image_cursors);
  ~PlatformDisplayMirror() override;

 private:
  // PlatformDisplayDefault:
  void OnAcceleratedWidgetAvailable(gfx::AcceleratedWidget widget,
                                    float device_scale_factor) override;

  Display* display_to_mirror_;
  DISALLOW_COPY_AND_ASSIGN(PlatformDisplayMirror);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_PLATFORM_DISPLAY_MIRROR_H_
