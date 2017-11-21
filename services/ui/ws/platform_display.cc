// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/platform_display.h"

#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "services/ui/ws/platform_display_default.h"
#include "services/ui/ws/platform_display_factory.h"
#include "services/ui/ws/platform_display_mirror.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/threaded_image_cursors.h"
#include "services/ui/ws/threaded_image_cursors_factory.h"

namespace ui {
namespace ws {

// static
PlatformDisplayFactory* PlatformDisplay::factory_ = nullptr;

// static
std::unique_ptr<PlatformDisplay> PlatformDisplay::Create(
    ServerWindow* root,
    const display::ViewportMetrics& metrics,
    ThreadedImageCursorsFactory* threaded_image_cursors_factory,
    Display* display_to_mirror) {
  if (factory_)
    return factory_->CreatePlatformDisplay(root, metrics);

  std::unique_ptr<ThreadedImageCursors> cursors;
#if !defined(OS_ANDROID)
  cursors = threaded_image_cursors_factory->CreateCursors();
#endif

  if (display_to_mirror) {
    return std::make_unique<PlatformDisplayMirror>(root, display_to_mirror,
                                                   metrics, std::move(cursors));
  }
  return std::make_unique<PlatformDisplayDefault>(root, metrics,
                                                  std::move(cursors));
}

}  // namespace ws
}  // namespace ui
