// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_button_screenshot_controller.h"

#include <memory>

#include "ash/accelerators/accelerator_controller.h"
#include "ash/public/interfaces/volume.mojom.h"
#include "ash/shell.h"
#include "ash/system/power/power_button_test_base.h"
#include "ash/system/tray/system_tray.h"
#include "ash/test_screenshot_delegate.h"
#include "base/test/simple_test_tick_clock.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace ash {

namespace {

// ash::mojom::VolumeController is implemented by chrome and used by ash.
// TestVolumeController is injected in ash instead to break chrome dependency
// for testing purpose.
class TestVolumeController : mojom::VolumeController {
 public:
  TestVolumeController() : binding_(this) {}
  ~TestVolumeController() override = default;

  mojom::VolumeControllerPtr CreateInterfacePtr() {
    mojom::VolumeControllerPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  void FlushForTesting() { binding_.FlushForTesting(); }

  void VolumeMute() override {}
  void VolumeDown() override { ++handle_volume_down_count_; }
  void VolumeUp() override { ++handle_volume_up_count_; }

  int handle_volume_down_count() const { return handle_volume_down_count_; }
  int handle_volume_up_count() const { return handle_volume_up_count_; }

 private:
  int handle_volume_down_count_ = 0;
  int handle_volume_up_count_ = 0;

  mojo::Binding<mojom::VolumeController> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestVolumeController);
};

}  // namespace

class PowerButtonScreenshotControllerTest : public PowerButtonTestBase {
 public:
  PowerButtonScreenshotControllerTest() = default;
  ~PowerButtonScreenshotControllerTest() override = default;

  void SetUp() override {
    PowerButtonTestBase::SetUp();
    InitPowerButtonControllerMembers(true /* send_accelerometer_update */);

    // Install TestVolumeController in ash for handling volume key accelerator.
    Shell::Get()->accelerator_controller()->SetVolumeController(
        volume_controller.CreateInterfacePtr());

    screenshot_delegate_ = GetScreenshotDelegate();
    screenshot_delegate_->set_can_take_screenshot(true);
    EnableTabletMode(true);

    // Advance a long duration from initialized last resume time in
    // TabletPowerButtonController to avoid cross interference.
    tick_clock_->Advance(base::TimeDelta::FromMilliseconds(3000));
  }

 protected:
  int GetScreenshotCount() const {
    return screenshot_delegate_->handle_take_screenshot_count();
  }

  int GetHandleVolumeDownCount() {
    volume_controller.FlushForTesting();
    return volume_controller.handle_volume_down_count();
  }

  int GetHandleVolumeUpCount() {
    volume_controller.FlushForTesting();
    return volume_controller.handle_volume_up_count();
  }

  TestVolumeController volume_controller;
  TestScreenshotDelegate* screenshot_delegate_ = nullptr;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(PowerButtonScreenshotControllerTest);
};

// Tests that power button screenshot accelerator works on tablet mode only.
TEST_F(PowerButtonScreenshotControllerTest, TabletMode) {
  // Press power button and volume down simultaneously takes a screenshot.
  PressKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  ReleasePowerButton();
  EXPECT_EQ(1, GetScreenshotCount());

  // Tests that screenshot handling is not active when not in tablet mode.
  EnableTabletMode(false);
  PressKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  ReleasePowerButton();
  EXPECT_EQ(1, GetScreenshotCount());
}

// Tests that if power-button/volume-down is released before
// volume-down/power-button is pressed, it does not take screenshot.
TEST_F(PowerButtonScreenshotControllerTest, ReleaseBeforeAnotherPressed) {
  // Release volume down before power button pressed.
  PressKey(ui::VKEY_VOLUME_DOWN);
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  ReleasePowerButton();
  EXPECT_EQ(0, GetScreenshotCount());

  // Release power button before volume down pressed.
  PressPowerButton();
  ReleasePowerButton();
  PressKey(ui::VKEY_VOLUME_DOWN);
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
}

// Tests when power button is pressed first and meets screenshot chord
// condition.
TEST_F(PowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_ScreenshotChord) {
  PressPowerButton();
  tick_clock_->Advance(PowerButtonScreenshotController::kScreenshotChordDelay -
                       base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  // Verify that screenshot is taken, volume down is consumed.
  EXPECT_EQ(1, GetScreenshotCount());
  EXPECT_EQ(0, GetHandleVolumeDownCount());
  // Keep pressing volume down key under screenshot chord condition will not
  // take screenshot again, volume down is also consumed.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(1));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(1, GetScreenshotCount());
  EXPECT_EQ(0, GetHandleVolumeDownCount());
  // Keep pressing volume down key off screenshot chord condition will not
  // take screenshot and still consume volume down event.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(1, GetScreenshotCount());
  EXPECT_EQ(0, GetHandleVolumeDownCount());

  // Release power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Release volume down key, and verify nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(1, GetScreenshotCount());
  EXPECT_EQ(0, GetHandleVolumeDownCount());
}

// Tests when power button is pressed first, and then volume down pressed but
// doesn't meet screenshot chord condition.
TEST_F(PowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_NoScreenshotChord) {
  PressPowerButton();
  tick_clock_->Advance(PowerButtonScreenshotController::kScreenshotChordDelay +
                       base::TimeDelta::FromMilliseconds(1));
  PressKey(ui::VKEY_VOLUME_DOWN);
  // Verify that screenshot is not taken, volume down is not consumed.
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_EQ(1, GetHandleVolumeDownCount());
  // Keep pressing volume down key should continue triggerring volume down.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_EQ(2, GetHandleVolumeDownCount());
  // Release power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Release volume down key, and verify nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_EQ(2, GetHandleVolumeDownCount());
}

TEST_F(PowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_VolumeKeyCancelTabletPowerButton) {
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->ShutdownTimerIsRunning());
  PressKey(ui::VKEY_VOLUME_DOWN);
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
