// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_LOCK_SCREEN_ACTION_TEST_LOCK_SCREEN_ACTION_BACKGROUND_CONTROLLER_H_
#define ASH_SYSTEM_LOCK_SCREEN_ACTION_TEST_LOCK_SCREEN_ACTION_BACKGROUND_CONTROLLER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/system/lock_screen_action/lock_screen_action_background_controller.h"
#include "base/macros.h"

namespace aura {
class Window;
}

namespace views {
class Widget;
}

namespace ash {

// Testing implementation of LockScreenActionBackgroundController.
// This implementation uses a window (if created at all) that does not animate
// visibility. When |ShowBackground| and |HideBackground| methods are called,
// the controller will transition to kShowing and kHiding states. The tests
// should use |FinishShow| and |FinishHide| to complete visibility transitions.
class ASH_EXPORT TestLockScreenActionBackgroundController
    : public LockScreenActionBackgroundController {
 public:
  explicit TestLockScreenActionBackgroundController(bool create_testing_window);
  ~TestLockScreenActionBackgroundController() override;

  // LockScreenBackgroundController:
  bool IsBackgroundWindow(aura::Window* window) const override;
  bool ShowBackground() override;
  bool HideBackgroundImmediately() override;
  bool HideBackground() override;

  // If the controller is in kShowing state, updates state to kShown.
  // Returns whether the state has changed.
  bool FinishShow();

  // If the controller is in kHiding state, updates state to kHidden, and hides
  // testing window is a window was created.
  bool FinishHide();

  // Gets the testing window, if one was created.
  const aura::Window* GetWindow() const;

 private:
  // Whether the testing background controller should create testing window.
  bool create_testing_window_;

  // Testing widget created when the background is shown.
  // Note that this will always be nullptr if |create_testing_window_| is false.
  std::unique_ptr<views::Widget> widget_;

  DISALLOW_COPY_AND_ASSIGN(TestLockScreenActionBackgroundController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_LOCK_SCREEN_ACTION_TEST_LOCK_SCREEN_ACTION_BACKGROUND_CONTROLLER_H_
