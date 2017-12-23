// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_POWER_OFF_MENU_CONTROLLER_H_
#define ASH_SYSTEM_POWER_POWER_OFF_MENU_CONTROLLER_H_

#include "ash/ash_export.h"
#include "base/timer/timer.h"

namespace gfx {
class Rect;
}

namespace views {
class Widget;
}

namespace ash {
class PowerButtonDisplayController;

class ASH_EXPORT PowerOffMenuController {
 public:
  // Time that power button should be pressed before to show the power off menu.
  static constexpr base::TimeDelta kPowerOffMenuTimeout =
      base::TimeDelta::FromSeconds(3);

  // Amount of time since last screen state change that power button event needs
  // to be ignored.
  static constexpr base::TimeDelta kScreenStateChangeDelay =
      base::TimeDelta::FromMilliseconds(500);

  // Ignore button-up events occurring within this many milliseconds of the
  // previous button-up event. This prevents us from falling behind if the power
  // button is pressed repeatedly.
  static constexpr base::TimeDelta kIgnoreRepeatedButtonUpDelay =
      base::TimeDelta::FromMilliseconds(500);

  PowerOffMenuController(PowerButtonDisplayController* display_controller);
  ~PowerOffMenuController();

  // Handles a power button event.
  void OnPowerButtonEvent(bool down, const base::TimeTicks& timestamp);

  // True if the power off menu is opened.
  bool IsMenuOpened() const;

  // Dismisses the opened power off menu.
  void DismissMenu();

 private:
  friend class PowerOffMenuControllerTestApi;

  // Called by |power_off_menu_timer_| to start showing the power off menu.
  void OnPowerOffMenuTimeout();

  // Creates the fullscreen widget responsible for showing the power off menu.
  std::unique_ptr<views::Widget> CreateFullscreenWidget();

  // Used to interact with the display.
  PowerButtonDisplayController* display_controller_;  // Not owned

  // Started when the power button is pressed in tablet mode and stopped when
  // it's released. Runs OnPowerOffMenuTimeout() to show the power off menu.
  base::OneShotTimer power_off_menu_timer_;

  std::unique_ptr<views::Widget> fullscreen_widget_;

  // True if the screen was off when the power button was pressed.
  bool screen_off_when_power_button_down_ = false;

  // Saves the most recent timestamp that power button is released.
  base::TimeTicks last_button_up_time_;

  // True if power button released should force off display.
  bool force_off_on_button_up_ = true;

  DISALLOW_COPY_AND_ASSIGN(PowerOffMenuController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_POWER_OFF_MENU_CONTROLLER_H_
