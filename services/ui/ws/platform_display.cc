// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/platform_display.h"

#include "base/memory/ptr_util.h"
#include "services/ui/ws/platform_display_default.h"
#include "services/ui/ws/platform_display_factory.h"
#include "services/ui/ws/platform_display_unified.h"
#include "services/ui/ws/server_window.h"
#include "services/ui/ws/threaded_image_cursors.h"
#include "services/ui/ws/threaded_image_cursors_factory.h"
#include "ui/display/manager/display_manager.h"

namespace ui {
namespace ws {

// static
PlatformDisplayFactory* PlatformDisplay::factory_ = nullptr;

// static
std::unique_ptr<PlatformDisplay> PlatformDisplay::Create(
    ServerWindow* root,
    const display::Display& display,
    const display::ViewportMetrics& metrics,
    ThreadedImageCursorsFactory* threaded_image_cursors_factory) {
  if (factory_)
    return factory_->CreatePlatformDisplay(root, metrics);

#if defined(OS_ANDROID)
  return base::MakeUnique<PlatformDisplayDefault>(root, metrics,
                                                  nullptr /* image_cursors */);
#else
  if (display.id() == display::DisplayManager::kUnifiedDisplayId) {
    return base::MakeUnique<PlatformDisplayUnified>(
        root, metrics, threaded_image_cursors_factory->CreateCursors());
  }
  return base::MakeUnique<PlatformDisplayDefault>(
      root, metrics, threaded_image_cursors_factory->CreateCursors());
#endif
}

}  // namespace ws
}  // namespace ui
