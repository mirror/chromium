// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_TABLET_POWER_BUTTON_CONTROLLER_H_
#define ASH_SYSTEM_POWER_TABLET_POWER_BUTTON_CONTROLLER_H_

#include "ash/ash_export.h"
#include "base/timer/timer.h"
#include "chromeos/dbus/power_manager_client.h"

namespace gfx {
class Rect;
}  // namespace gfx

namespace views {
class Widget;
}  // namespace views

namespace base {
class TickClock;
}  // namespace base

namespace ash {
class PowerButtonDisplayController;

class ASH_EXPORT TabletPowerButtonController
    : public chromeos::PowerManagerClient::Observer {
 public:
  // Time that power button should be pressed before to start showing the power
  // off menu animation.
  static constexpr base::TimeDelta kStartPowerOffMenuAnimationTimeout =
      base::TimeDelta::FromMilliseconds(500);

  TabletPowerButtonController(PowerButtonDisplayController* display_controller,
                              base::TickClock* tick_clock);
  ~TabletPowerButtonController() override;

  // Handles a power button event.
  void OnPowerButtonEvent(bool down, const base::TimeTicks& timestamp);

  // True if the menu is opened.
  bool IsMenuOpened() const;

  // Dismisses the menu.
  void DismissMenu();

  // Cancel the ongoing power button behavior of tablet.
  void CancelTabletPowerButton();

 private:
  friend class TabletPowerButtonControllerTestApi;

  // Called by |power_off_menu_timer_| to start showing the power off menu.
  void OnPowerOffMenuTimeout();

  // Creates the fullscreen widget responsible for showing the power off menu.
  std::unique_ptr<views::Widget> CreateFullscreenWidget();

  // chromeos::PowerManagerClient::Observer:
  void SuspendDone(const base::TimeDelta& sleep_duration) override;

  // Used to interact with the display.
  PowerButtonDisplayController* display_controller_;  // Not owned

  // Started when the power button is pressed and stopped when it's released.
  // Runs OnPowerOffMenuTimeout() to show the power off menu.
  base::OneShotTimer power_off_menu_timer_;

  std::unique_ptr<views::Widget> fullscreen_widget_;

  // True if the screen was off when the power button was pressed.
  bool screen_off_when_power_button_down_ = false;

  // Saves the most recent timestamp that power button is released.
  base::TimeTicks last_button_up_time_;

  // True if power button released should force off display.
  bool force_off_on_button_up_ = true;

  // Saves the most recent timestamp that powerd is resuming from suspend,
  // updated in SuspendDone().
  base::TimeTicks last_resume_time_;

  // Time source for performed action times.
  base::TickClock* tick_clock_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(TabletPowerButtonController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_TABLET_POWER_BUTTON_CONTROLLER_H_
