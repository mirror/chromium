// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_button_controller_util.h"

#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/wm/lock_state_controller.h"

namespace ash {

// static
void PowerButtonControllerUtil::LockScreenIfRequired() {
  SessionController* session_controller = Shell::Get()->session_controller();
  LockStateController* lock_state_controller =
      Shell::Get()->lock_state_controller();
  if (session_controller->ShouldLockScreenAutomatically() &&
      session_controller->CanLockScreen() &&
      !session_controller->IsUserSessionBlocked() &&
      !lock_state_controller->LockRequested()) {
    lock_state_controller->LockWithoutAnimation();
  }
}

}  // namespace ash
