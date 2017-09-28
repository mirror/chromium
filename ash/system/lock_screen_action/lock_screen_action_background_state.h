// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_STATE_H_
#define ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_STATE_H_

namespace ash {

// The state of a lock screen action background widget - a widget shown when a
// lock screen action is activated behind the action window.
enum class LockScreenActionBackgroundState {
  // The background widget is not shown.
  kHidden,

  // The background widget is visible, and it's showing animation is in
  // progress - the widget has a ink drop animation that's activated when the
  // widget is shown. The widget will remain in kShowing state until the in drop
  // animation ends.
  kShowing,

  // The background widget is shown, and the ink drop associate with showing the
  // widget ended.
  kShown,

  // The background is being hidden - upon hiding the widget, ink drop activated
  // when the widget was shown is moved to hidden state. The widget will remain
  // visible, in kHiding state, until the ink drop animation ends.
  kHiding
};

}  // namespace ash

#endif  // ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_STATE_H_
