// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/exo/compositor_rotation_lock.h"

#include "base/trace_event/trace_event.h"
#include "components/exo/shell_surface.h"
#include "ui/gfx/geometry/size.h"

namespace exo {

// static
Orientation CompositorRotationLock::SizeToOrientation(const gfx::Size& size) {
  DCHECK_NE(size.width(), size.height());
  return size.width() > size.height() ? Orientation::LANDSCAPE
                                      : Orientation::PORTRAIT;
}

CompositorRotationLock::CompositorRotationLock(ShellSurface* shell_surface,
                                               Orientation expected_orientation)
    : shell_surface_(shell_surface),
      expected_orientation_(expected_orientation) {
  TRACE_EVENT_ASYNC_BEGIN1(
      "exo", "CompositorRotationLock", this, "expected_orientation",
      expected_orientation == Orientation::LANDSCAPE ? "landscape"
                                                     : "portrait");
}

CompositorRotationLock::~CompositorRotationLock() {
  Unlock();
  TRACE_EVENT_ASYNC_END0("exo", "CompositorRotationLock", this);
}

void CompositorRotationLock::Lock() {
  if (!unlocked_ && !compositor_lock_)
    compositor_lock_ = shell_surface_->CreateCompositorLock(this);
}

bool CompositorRotationLock::UnlockIfOrientationMatches(
    Orientation orientation) {
  if (expected_orientation_ == orientation) {
    Unlock();
    return true;
  }
  return false;
}

void CompositorRotationLock::Unlock() {
  if (compositor_lock_) {
    unlocked_ = true;
    compositor_lock_ = nullptr;
  }
}

void CompositorRotationLock::CompositorLockTimedOut() {
  Unlock();
}

}  // namespace exo
