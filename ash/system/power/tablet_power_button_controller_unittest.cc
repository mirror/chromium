// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/tablet_power_button_controller.h"

#include "ash/login_status.h"
#include "ash/public/cpp/ash_switches.h"
#include "ash/shell.h"
#include "ash/system/power/power_button_test_base.h"
#include "ash/system/power/power_button_util.h"
#include "ash/system/power/tablet_power_button_controller_test_api.h"
#include "base/command_line.h"
#include "base/test/simple_test_tick_clock.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "ui/events/test/event_generator.h"

namespace ash {

using NoTabletPowerButtonControllerTest = PowerButtonTestBase;

// Tests that |tablet_controller_| will only be created for tablet/detachable
// device.
TEST_F(NoTabletPowerButtonControllerTest, NotTablet) {
  ASSERT_FALSE(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kIsTabletOrDetachableDevice));
  EXPECT_FALSE(tablet_controller_);
}

class TabletPowerButtonControllerTest : public PowerButtonTestBase {
 public:
  TabletPowerButtonControllerTest() { set_has_tablet_switch(true); }
  ~TabletPowerButtonControllerTest() override = default;

  void SetUp() override {
    PowerButtonTestBase::SetUp();

    InitPowerButtonControllerMembers(true /* send_accelerometer_update */);
    EnableTabletMode(true);

    // Advance a long duration from initialized last resume time in
    // |tablet_controller_| to avoid cross interference.
    tick_clock_->Advance(base::TimeDelta::FromMilliseconds(3000));
  }

  DISALLOW_COPY_AND_ASSIGN(TabletPowerButtonControllerTest);
};

// Tests that the power off menu will not be shown if the power button is
// released quickly.
TEST_F(TabletPowerButtonControllerTest,
       ReleasePowerButtonBeforeShowPowerOffMenu) {
  // Tap the power button when screen is on will make screen off.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->PowerOffMenuTimerIsRunning());
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  ReleasePowerButton();
  power_manager_client_->SendBrightnessChanged(0, true);
  EXPECT_FALSE(tablet_test_api_->PowerOffMenuTimerIsRunning());
  EXPECT_TRUE(power_manager_client_->backlights_forced_off());

  AdvanceClockToAvoidIgnoring();
  // Tap the power button when screen is off will make screen on.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->PowerOffMenuTimerIsRunning());
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  ReleasePowerButton();
  EXPECT_FALSE(tablet_test_api_->PowerOffMenuTimerIsRunning());
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
}

// Tests that tap outside of the menu bounds will dimiss the menu.
TEST_F(TabletPowerButtonControllerTest, TapToDismissMenu) {
  // Long press the power button to show the menu.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->PowerOffMenuTimerIsRunning());
  EXPECT_TRUE(tablet_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(tablet_controller_);
  EXPECT_TRUE(tablet_test_api_->IsMenuOpened());

  gfx::Rect menu_bounds = tablet_test_api_->GetMenuBoundsInScreen();
  GetEventGenerator().GestureTapAt(
      gfx::Point(menu_bounds.x() - 5, menu_bounds.y() - 5));
  EXPECT_FALSE(tablet_test_api_->IsMenuOpened());
}

// Tests the menu items according to the login status.
TEST_F(TabletPowerButtonControllerTest, MenuItemsToLoginStatus) {
  // No sign out item if user is not login.
  ClearLogin();
  Shell::Get()->UpdateAfterLoginStatusChange(LoginStatus::NOT_LOGGED_IN);
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(tablet_test_api_->IsMenuOpened());
  EXPECT_FALSE(tablet_test_api_->HasSignOut());

  // Tap to dismiss the menu.
  gfx::Rect menu_bounds = tablet_test_api_->GetMenuBoundsInScreen();
  GetEventGenerator().GestureTapAt(
      gfx::Point(menu_bounds.x() - 5, menu_bounds.y() - 5));
  EXPECT_FALSE(tablet_test_api_->IsMenuOpened());
  // Should have sign out item if user is login.
  CreateUserSessions(1);
  Shell::Get()->UpdateAfterLoginStatusChange(LoginStatus::USER);
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(tablet_test_api_->IsMenuOpened());
  EXPECT_TRUE(tablet_test_api_->HasSignOut());

  // Tap to dismiss the menu.
  menu_bounds = tablet_test_api_->GetMenuBoundsInScreen();
  GetEventGenerator().GestureTapAt(
      gfx::Point(menu_bounds.x() - 5, menu_bounds.y() - 5));
  EXPECT_FALSE(tablet_test_api_->IsMenuOpened());
  // Should have sign out item if user is login but screen is locked.
  Shell::Get()->UpdateAfterLoginStatusChange(LoginStatus::LOCKED);
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(tablet_test_api_->IsMenuOpened());
  EXPECT_TRUE(tablet_test_api_->HasSignOut());
}

// Repeat long press should redisplay the menu.
TEST_F(TabletPowerButtonControllerTest, PressButtonWhenMenuIsOpened) {
  // Long press the power button to show the menu.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(tablet_test_api_->IsMenuOpened());

  AdvanceClockToAvoidIgnoring();
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->PowerOffMenuTimerIsRunning());
  EXPECT_TRUE(tablet_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(tablet_test_api_->IsMenuOpened());
}

// Tests that attach the detached detachable device should dismiss the opened
// power off menu.
TEST_F(TabletPowerButtonControllerTest, AttachTheTabletShouldDismissMenu) {
  // Long press the power button to show the menu.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(tablet_test_api_->IsMenuOpened());

  // Change from tablet mode to clamshell mode.
  EnableTabletMode(false);
  EXPECT_FALSE(tablet_test_api_->IsMenuOpened());
}

// Tests that screen changes to idle off will dismiss the opened menu.
TEST_F(TabletPowerButtonControllerTest, DismissMenuWhenScreenIsIdleOff) {
  // Long press the power button to show the menu.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(tablet_test_api_->IsMenuOpened());

  // Mock screen idle off.
  power_manager_client_->SendBrightnessChanged(0, true);
  EXPECT_FALSE(tablet_test_api_->IsMenuOpened());
}

// Tests that tapping the power button should dimiss the opened menu.
TEST_F(TabletPowerButtonControllerTest, TappingPowerButtonWhenMenuIsOpened) {
  // Long press the power button to show the menu.
  PressPowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  EXPECT_TRUE(tablet_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(tablet_test_api_->IsMenuOpened());

  // Tapping the power button when menu is opened will dismiss the menu.
  AdvanceClockToAvoidIgnoring();
  PressPowerButton();
  ReleasePowerButton();
  power_manager_client_->SendBrightnessChanged(0, true);
  EXPECT_TRUE(power_manager_client_->backlights_forced_off());
  EXPECT_FALSE(tablet_test_api_->IsMenuOpened());

  // Long press the power button when backlights is off will show the menu.
  PressPowerButton();
  power_manager_client_->SendBrightnessChanged(kNonZeroBrightness, true);
  EXPECT_TRUE(tablet_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(tablet_test_api_->IsMenuOpened());
  // Tapping the power button will dismiss the menu.
  AdvanceClockToAvoidIgnoring();
  PressPowerButton();
  ReleasePowerButton();
  power_manager_client_->SendBrightnessChanged(0, true);
  EXPECT_TRUE(power_manager_client_->backlights_forced_off());
  EXPECT_FALSE(tablet_test_api_->IsMenuOpened());
}

// Tests that suspend will dismiss the opened menu.
TEST_F(TabletPowerButtonControllerTest, SuspendWithMenuOn) {
  // Long press the power button to show the menu.
  PressPowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  EXPECT_TRUE(tablet_test_api_->TriggerPowerOffMenuTimeout());
  ReleasePowerButton();
  EXPECT_TRUE(tablet_test_api_->IsMenuOpened());

  power_manager_client_->SendSuspendImminent(
      power_manager::SuspendImminent_Reason_OTHER);
  EXPECT_FALSE(tablet_test_api_->IsMenuOpened());
  power_manager_client_->SendSuspendDone();
  EXPECT_FALSE(tablet_test_api_->IsMenuOpened());
}

}  // namespace ash
