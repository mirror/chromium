// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_EXO_COMPOSITOR_ROTATION_LOCK_H_
#define COMPONENTS_EXO_COMPOSITOR_ROTATION_LOCK_H_

#include <memory>

#include "base/macros.h"
#include "ui/compositor/compositor_lock.h"

namespace gfx {
class Size;
}

namespace exo {
class ShellSurface;
enum class Orientation;

// Used to prevent further resizes while a resize is pending.
class CompositorRotationLock
    : NON_EXPORTED_BASE(public ui::CompositorLockClient) {
 public:
  static Orientation SizeToOrientation(const gfx::Size& size);

  // |expected_orientation| has to be either LANDSCAPE or PORTRAIT.
  CompositorRotationLock(ShellSurface* shell_surface,
                         Orientation expected_orientation);
  ~CompositorRotationLock() override;

  // Lock the compositor.
  void Lock();

  // Unlock compositor if the new_size matches the expected orientation.
  bool UnlockIfOrientationMatches(Orientation orientation);

 private:
  void Unlock();

  // ui::CompositorLockClient implementation.
  void CompositorLockTimedOut() override;

  ShellSurface* shell_surface_;
  Orientation expected_orientation_;
  bool unlocked_ = false;
  std::unique_ptr<ui::CompositorLock> compositor_lock_;

  DISALLOW_COPY_AND_ASSIGN(CompositorRotationLock);
};

}  // namespace exo

#endif  // COMPONENTS_EXO_COMPOSITOR_ROTATION_LOCK_H_
