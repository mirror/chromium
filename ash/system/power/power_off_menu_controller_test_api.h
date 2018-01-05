// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_TABLET_POWER_BUTTON_CONTROLLER_TEST_API_H_
#define ASH_SYSTEM_POWER_TABLET_POWER_BUTTON_CONTROLLER_TEST_API_H_

#include "base/compiler_specific.h"
#include "base/macros.h"

namespace gfx {
class Rect;
}  // namespace gfx

namespace ash {
class TabletPowerButtonController;
class PowerOffMenuView;

class TabletPowerButtonControllerTestApi {
 public:
  TabletPowerButtonControllerTestApi(TabletPowerButtonController* controller);
  ~TabletPowerButtonControllerTestApi();

  // Returns true when |power_off_menu_timer_| is running.
  bool PowerOffMenuTimerIsRunning() const;

  // If |controller_->power_off_menu_timer_| is running, stops it, runs its
  // task, and returns true. Otherwise, returns false.
  bool TriggerPowerOffMenuTimeout() WARN_UNUSED_RESULT;

  gfx::Rect GetMenuBoundsInScreen() const;
  PowerOffMenuView* GetPowerOffMenuView() const;
  bool IsMenuOpened() const;
  bool HasSignOut() const;

 private:
  TabletPowerButtonController* controller_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(TabletPowerButtonControllerTestApi);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_TABLET_POWER_BUTTON_CONTROLLER_TEST_API_H_
