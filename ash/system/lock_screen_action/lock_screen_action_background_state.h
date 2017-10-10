// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_STATE_H_
#define ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_STATE_H_

#include "ash/ash_export.h"

namespace ash {

// The state of lock screen action background - the background is shown when a
// lock screen action is activated. It's shown behind the action window and
// its visibility changes can be animated.
enum class ASH_EXPORT LockScreenActionBackgroundState {
  // The background is not shown.
  kHidden,

  // The background is visible, and it's "show" animation is in progress (the
  // background can have an animation that's activated as the background is
  // shown). The background will remain in kShowing state until the animation
  // ends.
  kShowing,

  // The background is shown, and the animation associate with showing the
  // background has ended.
  kShown,

  // The background is being hidden. The background can have an animation that
  // is activates upon request to hide the background. The background will
  // remain visible, in kHiding state, until the "hide" animation ends.
  kHiding
};

}  // namespace ash

#endif  // ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_STATE_H_
