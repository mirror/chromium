// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_button_screenshot_controller.h"

#include <memory>

#include "ash/ash_switches.h"
#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/system/power/tablet_power_button_controller.h"
#include "ash/system/tray/system_tray.h"
#include "ash/test/ash_test_base.h"
#include "ash/test_screenshot_delegate.h"
#include "ash/wm/power_button_controller.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/command_line.h"
#include "base/test/simple_test_tick_clock.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "ui/events/event.h"
#include "ui/events/test/event_generator.h"

namespace ash {

namespace {

// Vector pointing up (e.g. keyboard in clamshell).
constexpr gfx::Vector3dF kUpVector = {0, 0,
                                      TabletPowerButtonController::kGravity};

// Vector pointing sideways (e.g. screen in 90-degree clamshell).
constexpr gfx::Vector3dF kSidewaysVector = {
    0, TabletPowerButtonController::kGravity, 0};

}  // namespace

class PowerButtonScreenshotControllerTest : public AshTestBase {
 public:
  PowerButtonScreenshotControllerTest() = default;
  ~PowerButtonScreenshotControllerTest() override = default;

  void SetUp() override {
    // This also initializes DBusThreadManager.
    std::unique_ptr<chromeos::DBusThreadManagerSetter> dbus_setter =
        chromeos::DBusThreadManager::GetSetterForTesting();
    power_manager_client_ = new chromeos::FakePowerManagerClient();
    dbus_setter->SetPowerManagerClient(base::WrapUnique(power_manager_client_));
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kAshEnableTabletMode);
    AshTestBase::SetUp();

    InitPowerButtonControllerMembers(true /* send_accelerometer_update */);

    screenshot_delegate_ = GetScreenshotDelegate();
    screenshot_delegate_->set_can_take_screenshot(true);

    EnableTabletMode(true);
    ASSERT_EQ(0, GetScreenshotCount());
  }

  void TearDown() override {
    const Config config = Shell::GetAshConfig();
    AshTestBase::TearDown();
    // Mash/mus shuts down dbus after each test.
    if (config == Config::CLASSIC)
      chromeos::DBusThreadManager::Shutdown();
  }

 protected:
  // Initializes |power_button_controller_| and other members that point at
  // objects owned by it. If |send_accelerometer_update| is true, an
  // accelerometer update is sent so that a PowerButtonScreenshotController will
  // be created.
  void InitPowerButtonControllerMembers(bool send_accelerometer_update) {
    power_button_controller_ = Shell::Get()->power_button_controller();
    tick_clock_ = new base::SimpleTestTickClock;
    power_button_controller_->SetTickClockForTesting(
        base::WrapUnique(tick_clock_));

    if (send_accelerometer_update) {
      SendAccelerometerUpdate(kSidewaysVector, kUpVector);
      screenshot_controller_ =
          power_button_controller_
              ->power_button_screenshot_controller_for_test();
    } else {
      screenshot_controller_ = nullptr;
    }
  }

  // Sends an update with screen and keyboard accelerometer readings to
  // PowerButtonController.
  void SendAccelerometerUpdate(const gfx::Vector3dF& screen,
                               const gfx::Vector3dF& keyboard) {
    scoped_refptr<chromeos::AccelerometerUpdate> update(
        new chromeos::AccelerometerUpdate());
    update->Set(chromeos::ACCELEROMETER_SOURCE_SCREEN, screen.x(), screen.y(),
                screen.z());
    update->Set(chromeos::ACCELEROMETER_SOURCE_ATTACHED_KEYBOARD, keyboard.x(),
                keyboard.y(), keyboard.z());

    power_button_controller_->OnAccelerometerUpdated(update);
  }

  void PressPowerButton() {
    power_button_controller_->PowerButtonEventReceived(true,
                                                       tick_clock_->NowTicks());
  }

  void ReleasePowerButton() {
    power_button_controller_->PowerButtonEventReceived(false,
                                                       tick_clock_->NowTicks());
  }

  void PressKey(ui::KeyboardCode key_code) {
    GetEventGenerator().PressKey(key_code, ui::EF_NONE);
  }

  void ReleaseKey(ui::KeyboardCode key_code) {
    GetEventGenerator().ReleaseKey(key_code, ui::EF_NONE);
  }

  void EnableTabletMode(bool enable) {
    Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(
        enable);
  }

  int GetScreenshotCount() const {
    return screenshot_delegate_->handle_take_screenshot_count();
  }

  // Ownership is passed on to chromeos::DBusThreadManager.
  chromeos::FakePowerManagerClient* power_manager_client_ = nullptr;

  PowerButtonController* power_button_controller_ = nullptr;  // Not owned.
  TestScreenshotDelegate* screenshot_delegate_ = nullptr;     // Not owned.
  PowerButtonScreenshotController* screenshot_controller_ =
      nullptr;                                       // Not owned.
  base::SimpleTestTickClock* tick_clock_ = nullptr;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(PowerButtonScreenshotControllerTest);
};

TEST_F(PowerButtonScreenshotControllerTest, Basic) {
  // Tests that pressing power button alone does not take a screenshot.
  PressPowerButton();
  ReleasePowerButton();
  EXPECT_EQ(0, GetScreenshotCount());

  // Tests that pressing power button and volume down simultaneously takes a
  // screenshot, but the pressing order doesn't matter.
  PressKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  ReleasePowerButton();
  EXPECT_EQ(1, GetScreenshotCount());

  PressPowerButton();
  PressKey(ui::VKEY_VOLUME_DOWN);
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  ReleasePowerButton();
  EXPECT_EQ(2, GetScreenshotCount());

  // Tests that press & release volume down then pressing power button does not
  // take a screenshot.
  PressKey(ui::VKEY_VOLUME_DOWN);
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  ReleasePowerButton();
  EXPECT_EQ(2, GetScreenshotCount());

  // Tests that screenshot handling is not active when not in tablet mode.
  EnableTabletMode(false);
  PressKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  ReleasePowerButton();
  EXPECT_EQ(2, GetScreenshotCount());
}

// Tests that volume down key event is properly updated in
// PowerButtonScreenshotController when system tray bubble is shown. This is a
// regression test for crbug.com/765473.
TEST_F(PowerButtonScreenshotControllerTest, VolumeDownKeyWithTrayBubbleShown) {
  // Simulates that pressing volume down key triggers volume bubble view.
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_TRUE(screenshot_controller_->volume_key_pressed());
  SystemTray* tray = GetPrimarySystemTray();
  tray->ShowDefaultView(BUBBLE_CREATE_NEW, false /* show_by_click */);
  // Release volume down key while tray bubble is still shown.
  ASSERT_TRUE(tray->IsSystemBubbleVisible());
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(screenshot_controller_->volume_key_pressed());
  // Now press power button, verify that it doesn't do screenshot.
  PressPowerButton();
  ReleasePowerButton();
  EXPECT_EQ(0, GetScreenshotCount());
  tray->CloseBubble();
}

}  // namespace ash
