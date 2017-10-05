// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_button_screenshot_controller.h"

#include <memory>

#include "ash/accelerators/accelerator_controller.h"
#include "ash/public/interfaces/volume.mojom.h"
#include "ash/shell.h"
#include "ash/system/power/power_button_test_base.h"
#include "ash/test_screenshot_delegate.h"
#include "ash/wm/lock_state_controller_test_api.h"
#include "ash/wm/power_button_controller.h"
#include "base/test/simple_test_tick_clock.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace ash {

namespace {

// Implementation used to track volume down key's stopped propagation count.
class TestObserver : public PowerButtonScreenshotController::TestObserver {
 public:
  TestObserver() = default;
  ~TestObserver() override = default;

  // PowerButtonScreenshotController::TestObserver:
  void OnStoppedPropagation() override { ++stopped_propagation_count_; }

  int stopped_propagation_count() const { return stopped_propagation_count_; }

 private:
  int stopped_propagation_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

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
    // Set |test_observer_| in |screenshot_controller_| to observe key event
    // stopped propagation.
    test_observer_ = new TestObserver;
    screenshot_controller_->SetObserverForTesting(
        base::WrapUnique(test_observer_));

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

  // Returns volume down key handled count by |volume_controller|.
  int GetHandleVolumeDownCount() {
    // |volume_controller|'s VolumeDown() is mojo async call.
    volume_controller.FlushForTesting();
    return volume_controller.handle_volume_down_count();
  }

  // Returns volume up key handled count by |volume_controller|.
  int GetHandleVolumeUpCount() {
    // |volume_controller|'s VolumeUp() is mojo async call.
    volume_controller.FlushForTesting();
    return volume_controller.handle_volume_up_count();
  }

  TestVolumeController volume_controller;
  TestScreenshotDelegate* screenshot_delegate_ = nullptr;  // Not owned.
  TestObserver* test_observer_ = nullptr;                  // Not owned.

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
  EXPECT_EQ(1, test_observer_->stopped_propagation_count());
  // Keep pressing volume down key under screenshot chord condition will not
  // take screenshot again, volume down is also consumed.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(1));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(1, GetScreenshotCount());
  EXPECT_EQ(2, test_observer_->stopped_propagation_count());
  // Keep pressing volume down key off screenshot chord condition will not
  // take screenshot and still consume volume down event.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(1, GetScreenshotCount());
  EXPECT_EQ(3, test_observer_->stopped_propagation_count());

  // Release power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Release volume down key, and verify nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(1, GetScreenshotCount());
  EXPECT_EQ(3, test_observer_->stopped_propagation_count());
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
  EXPECT_EQ(0, test_observer_->stopped_propagation_count());
  EXPECT_EQ(1, GetHandleVolumeDownCount());
  // Keep pressing volume down key should continue triggerring volume down.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_EQ(0, test_observer_->stopped_propagation_count());
  EXPECT_EQ(2, GetHandleVolumeDownCount());
  // Release power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Release volume down key, and verify nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_EQ(0, test_observer_->stopped_propagation_count());
  EXPECT_EQ(2, GetHandleVolumeDownCount());
}

// Tests that volume key pressed cancels the ongoing tablet power button.
TEST_F(PowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_VolumeKeyCancelTabletPowerButton) {
  // Tests volume down key can stop tablet power button's shutdown timer.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->ShutdownTimerIsRunning());
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(tablet_test_api_->ShutdownTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());

  // Tests volume down key can stop shutdown animation timer.
  PressPowerButton();
  tablet_test_api_->TriggerShutdownTimeout();
  EXPECT_TRUE(lock_state_test_api_->shutdown_timer_is_running());
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(lock_state_test_api_->shutdown_timer_is_running());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());

  // Tests volume up key can stop tablet power button's shutdown timer.
  // Also tests that volume up key is not consumed.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->ShutdownTimerIsRunning());
  PressKey(ui::VKEY_VOLUME_UP);
  EXPECT_EQ(1, GetHandleVolumeUpCount());
  EXPECT_FALSE(tablet_test_api_->ShutdownTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());

  // Tests volume up key can stop shutdown animation timer.
  // Also tests that volume up key is not consumed.
  PressPowerButton();
  tablet_test_api_->TriggerShutdownTimeout();
  EXPECT_TRUE(lock_state_test_api_->shutdown_timer_is_running());
  PressKey(ui::VKEY_VOLUME_UP);
  EXPECT_EQ(2, GetHandleVolumeUpCount());
  EXPECT_FALSE(lock_state_test_api_->shutdown_timer_is_running());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
}

TEST_F(PowerButtonScreenshotControllerTest,
       VolumeDownPressedFirst_ScreenshotChord) {
  // Tests when volume down pressed first, it waits for power button pressed
  // screenshot chord.
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(1, test_observer_->stopped_propagation_count());
  // Press power button under screenshot chord condition, verify that screenshot
  // is taken.
  tick_clock_->Advance(PowerButtonScreenshotController::kScreenshotChordDelay -
                       base::TimeDelta::FromMilliseconds(2));
  PressPowerButton();
  EXPECT_EQ(1, GetScreenshotCount());
  // Keep pressing volume down key under screenshot chord condition will not
  // take screenshot again, volume down is also consumed.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(1));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(1, GetScreenshotCount());
  EXPECT_EQ(2, test_observer_->stopped_propagation_count());
  // Keep pressing volume down key off screenshot chord condition will not take
  // screenshot and still consume volume down event.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(1, GetScreenshotCount());
  EXPECT_EQ(3, test_observer_->stopped_propagation_count());

  // Release power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Release volume down key, and verify nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(1, GetScreenshotCount());
  EXPECT_EQ(3, test_observer_->stopped_propagation_count());
}

TEST_F(PowerButtonScreenshotControllerTest,
       VolumeDownPressedFirst_NoScreenshotChord) {
  // Tests when volume down pressed first, it waits for power button pressed
  // screenshot chord.
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(1, test_observer_->stopped_propagation_count());
  // Advance |tick_clock_| to off screenshot chord point. This will also trigger
  // volume down timer timeout. Verify there is a volume down operation.
  tick_clock_->Advance(PowerButtonScreenshotController::kScreenshotChordDelay +
                       base::TimeDelta::FromMilliseconds(1));
  EXPECT_TRUE(screenshot_controller_->TriggerVolumeDownTimerForTesting());
  EXPECT_EQ(1, GetHandleVolumeDownCount());
  // Press power button would not take screenshot.
  PressPowerButton();
  EXPECT_EQ(0, GetScreenshotCount());
  // Keep pressing volume down key should continue triggerring volume down.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_EQ(1, test_observer_->stopped_propagation_count());
  EXPECT_EQ(2, GetHandleVolumeDownCount());
  // Release power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Release volume down key, and verify nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_EQ(1, test_observer_->stopped_propagation_count());
  EXPECT_EQ(2, GetHandleVolumeDownCount());
}

TEST_F(PowerButtonScreenshotControllerTest,
       VolumeKeyPressedFirst_CancelTabletPowerButton) {
  // Tests volume down key invalidates tablet power button behavior.
  PressKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  EXPECT_FALSE(tablet_test_api_->ShutdownTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());

  // Tests volume up key invalidates tablet power button behavior. Also tests
  // that volume up key is not consumed.
  PressKey(ui::VKEY_VOLUME_UP);
  PressPowerButton();
  EXPECT_EQ(1, GetHandleVolumeUpCount());
  EXPECT_FALSE(tablet_test_api_->ShutdownTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
}

// Forced clamshell power button: tests that if power button is pressed first
TEST_F(PowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_ForcedClamshell) {
  // ForceClamshellPowerButton();
  // SendAccelerometerUpdate(kSidewaysVector, kSidewaysVector);
  // Initialize(LoginStatus::USER);
  // PressPowerButton();
  //// Verify that lock animation is not running when waiting for volume key.
  ///And / Start lock animation if we are done with waiting.
  // EXPECT_FALSE(lock_state_test_api_->is_animating_lock());
  // EXPECT_TRUE(
  // power_button_controller_->TriggerClamshellScreenshotTimerForTesting());
  // EXPECT_TRUE(lock_state_test_api_->is_animating_lock());
  // ReleasePowerButton();
  // EXPECT_FALSE(lock_state_test_api_->is_animating_lock());
}

}  // namespace ash
