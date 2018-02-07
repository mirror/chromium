// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws2/window_service_delegate.h"

#include "ui/aura/window.h"

namespace ui {
namespace ws2 {

void WindowServiceDelegate::ClientRequestedWindowBoundsChange(
    aura::Window* window,
    const gfx::Rect& bounds) {
  window->SetBounds(bounds);
}

}  // namespace ws2
}  // namespace ui
