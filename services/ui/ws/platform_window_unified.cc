// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/platform_window_unified.h"

#include "base/logging.h"
#include "ui/platform_window/platform_window_delegate.h"

namespace ui {

PlatformWindowUnified::PlatformWindowUnified(PlatformWindowDelegate* delegate)
    : delegate_(delegate) {
  DCHECK(delegate);
}

PlatformWindowUnified::~PlatformWindowUnified() {}

void PlatformWindowUnified::Show() {}

void PlatformWindowUnified::Hide() {}

void PlatformWindowUnified::Close() {
  delegate_->OnClosed();
}

void PlatformWindowUnified::PrepareForShutdown() {}

void PlatformWindowUnified::SetBounds(const gfx::Rect& bounds) {
  // Even if the pixel bounds didn't change this call to the delegate should
  // still happen. The device scale factor may have changed which effectively
  // changes the bounds.
  bounds_ = bounds;
  delegate_->OnBoundsChanged(bounds);
}

gfx::Rect PlatformWindowUnified::GetBounds() {
  return bounds_;
}

void PlatformWindowUnified::SetTitle(const base::string16& title) {}

void PlatformWindowUnified::SetCapture() {}

void PlatformWindowUnified::ReleaseCapture() {}

void PlatformWindowUnified::ToggleFullscreen() {}

void PlatformWindowUnified::Maximize() {}

void PlatformWindowUnified::Minimize() {}

void PlatformWindowUnified::Restore() {}

void PlatformWindowUnified::SetCursor(PlatformCursor cursor) {}

void PlatformWindowUnified::MoveCursorTo(const gfx::Point& location) {}

void PlatformWindowUnified::ConfineCursorToBounds(const gfx::Rect& bounds) {}

PlatformImeController* PlatformWindowUnified::GetPlatformImeController() {
  return nullptr;
}

}  // namespace ui
