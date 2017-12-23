// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_off_menu_controller.h"

#include "ash/login_status.h"
#include "ash/shell.h"
#include "ash/system/power/power_button_test_base.h"
#include "ash/system/power/power_off_menu_controller_test_api.h"
#include "base/test/simple_test_tick_clock.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "ui/events/test/event_generator.h"

namespace ash {

class PowerOffMenuControllerTest : public PowerButtonTestBase {
 public:
  PowerOffMenuControllerTest() = default;
  ~PowerOffMenuControllerTest() override = default;

  void SetUp() override {
    PowerButtonTestBase::SetUp();

    InitPowerButtonControllerMembers(false /* send_accelerometer_update */);
    EnableTabletMode(true);

    // Advance a long duration from initialized last resume time in
    // |power_off_menu_controller_| to avoid cross interference.
    tick_clock_->Advance(base::TimeDelta::FromMilliseconds(3000));
  }

  DISALLOW_COPY_AND_ASSIGN(PowerOffMenuControllerTest);
};

// Tests that |power_off_menu_controller_| will only be created in tablet mode.
TEST_F(PowerOffMenuControllerTest, TabletMode) {
  // No |power_off_menu_controller_| in non-tablet mode, then press the power
  // button will never show the power off menu.
  EnableTabletMode(false);
  EXPECT_FALSE(power_off_menu_controller_);

  // |power_off_menu_controller_| is not nullptr in tablet mode.
  EnableTabletMode(true);
  EXPECT_TRUE(power_off_menu_controller_);
}

// Tests that the power off menu will not be shown if the power button is
// released quickly.
TEST_F(PowerOffMenuControllerTest, ReleasePowerButtonBeforeShowPowerOffMenu) {
  // Tap the power button when screen is on will make screen off.
  PressPowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->PowerOffMenuTimerIsRunning());
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  ReleasePowerButton();
  power_manager_client_->SendBrightnessChanged(0, true);
  EXPECT_FALSE(power_off_menu_test_api_->PowerOffMenuTimerIsRunning());
  EXPECT_TRUE(power_manager_client_->backlights_forced_off());

  // TODO, move kIgnoreRepeatedButtonUpDelay to more common class.
  // Advance more than |kIgnoreRepeatedButtonUpDelay| to avoid too soon repeated
  // button press.
  tick_clock_->Advance(
      TabletPowerButtonController::kIgnoreRepeatedButtonUpDelay +
      base::TimeDelta::FromMilliseconds(2));
  // Tap the power button when screen is off will make screen on.
  // tick_clock_->Advance(base::TimeDelta::FromMilliseconds(3000));
  PressPowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->PowerOffMenuTimerIsRunning());
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  ReleasePowerButton();
  EXPECT_FALSE(power_off_menu_test_api_->PowerOffMenuTimerIsRunning());
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
}

// Tests that tap outside of the menu bounds will dimiss the menu.
TEST_F(PowerOffMenuControllerTest, TapToDismissMenu) {
  PressPowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->PowerOffMenuTimerIsRunning());
  EXPECT_TRUE(power_off_menu_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(power_off_menu_controller_);
  EXPECT_TRUE(power_off_menu_test_api_->IsMenuOpened());

  gfx::Rect menu_bounds = power_off_menu_test_api_->GetMenuBoundsInScreen();
  GetEventGenerator().GestureTapAt(
      gfx::Point(menu_bounds.x() - 5, menu_bounds.y() - 5));
  EXPECT_FALSE(power_off_menu_test_api_->IsMenuOpened());
}

// Tests that the menu will only has power off if not login.
// lock, sign out, login
// Tests that the menu should not contain sign out if user is not logged in,
// otherwise it should have sign out item.
//
// Tests the menu items according to the login status.
TEST_F(PowerOffMenuControllerTest, MenuItemsToLoginStatus) {
  ClearLogin();
  Shell::Get()->UpdateAfterLoginStatusChange(LoginStatus::NOT_LOGGED_IN);
  PressPowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->IsMenuOpened());
  EXPECT_FALSE(power_off_menu_test_api_->HasSignOut());

  // Tap to dismiss the menu.
  gfx::Rect menu_bounds = power_off_menu_test_api_->GetMenuBoundsInScreen();
  GetEventGenerator().GestureTapAt(
      gfx::Point(menu_bounds.x() - 5, menu_bounds.y() - 5));
  EXPECT_FALSE(power_off_menu_test_api_->IsMenuOpened());
  //
  CreateUserSessions(1);
  Shell::Get()->UpdateAfterLoginStatusChange(LoginStatus::USER);
  PressPowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->IsMenuOpened());
  EXPECT_TRUE(power_off_menu_test_api_->HasSignOut());

  // Tap to dismiss the menu.
  menu_bounds = power_off_menu_test_api_->GetMenuBoundsInScreen();
  GetEventGenerator().GestureTapAt(
      gfx::Point(menu_bounds.x() - 5, menu_bounds.y() - 5));
  EXPECT_FALSE(power_off_menu_test_api_->IsMenuOpened());
  // In Locked menu.
  Shell::Get()->UpdateAfterLoginStatusChange(LoginStatus::LOCKED);
  PressPowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->IsMenuOpened());
  EXPECT_TRUE(power_off_menu_test_api_->HasSignOut());
}

// Convert from tablet mode to clamshell when menu is opened.
TEST_F(PowerOffMenuControllerTest, DismissMenuIfConvertFromTabletToClamshell) {
  PressPowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->IsMenuOpened());
  EnableTabletMode(false);
  EXPECT_FALSE(power_off_menu_test_api_);
}

// continue press the button when menu is opened.
TEST_F(PowerOffMenuControllerTest, PressButtonIfMenuIsOpened) {
  PressPowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->IsMenuOpened());
  PressPowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->IsMenuOpened());
  ReleasePowerButton();
  EXPECT_TRUE(power_off_menu_test_api_->IsMenuOpened());
}

}  // namespace ash
