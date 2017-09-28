// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_OBSERVER_H_
#define ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_OBSERVER_H_

#include "ash/system/lock_screen_action/lock_screen_action_background_state.h"

namespace ash {

// Used to observe the state of a lock screen action background widget.
// The widget is shown when a lock screen action is launched (currently, only
// lock screen note action is supproted).
class LockScreenActionBackgroundObserver {
 public:
  virtual ~LockScreenActionBackgroundObserver() {}

  // Called when the state of the tracked lock screen action background widget
  // changes. E.g. when the widget starts showing (with animation), or the
  // widget show animation ends.
  virtual void OnLockScreenActionBackgroundStateChanged(
      LockScreenActionBackgroundState state) = 0;
};

}  // namespace ash

#endif  // ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_OBSERVER_H_
