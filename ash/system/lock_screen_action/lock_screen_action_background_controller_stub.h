// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_CONTROLLER_STUB_H_
#define ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_CONTROLLER_STUB_H_

#include "ash/system/lock_screen_action/lock_screen_action_background_controller.h"
#include "base/macros.h"

namespace ash {

// Stub lock screen action background controller - used when no background
// widget is shown with the lock screen action app window, as is the case when
// --show-md-login flag is not set. The controller will forever remain in hidden
// state.
class LockScreenActionBackgroundControllerStub
    : public LockScreenActionBackgroundController {
 public:
  LockScreenActionBackgroundControllerStub();
  ~LockScreenActionBackgroundControllerStub() override;

  // LockScreenActionBackgroundController:
  bool IsBackgroundWindow(aura::Window* window) const override;
  bool ShowBackground() override;
  bool HideBackground() override;
  bool HideBackgroundImmediately() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LockScreenActionBackgroundControllerStub);
};

}  // namespace ash

#endif  // ASH_SYSTEM_LOCK_SCREEN_ACTION_LOCK_SCREEN_ACTION_BACKGROUND_CONTROLLER_STUB_H_
