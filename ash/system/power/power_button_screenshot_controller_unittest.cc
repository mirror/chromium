// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_button_screenshot_controller.h"

#include <memory>

#include "ash/login_status.h"
#include "ash/shell.h"
#include "ash/system/power/power_button_controller.h"
#include "ash/system/power/power_button_screenshot_controller_test_api.h"
#include "ash/system/power/power_button_test_base.h"
#include "ash/system/power/tablet_power_button_controller_test_api.h"
#include "ash/test_screenshot_delegate.h"
#include "ash/wm/lock_state_controller_test_api.h"
#include "base/test/simple_test_tick_clock.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "ui/events/event.h"

namespace ash {

// Test fixture used for testing power button screenshot behavior under tablet
// power button.
class PowerButtonScreenshotControllerTest : public PowerButtonTestBase {
 public:
  PowerButtonScreenshotControllerTest() = default;
  ~PowerButtonScreenshotControllerTest() override = default;

  // PowerButtonTestBase:
  void SetUp() override {
    PowerButtonTestBase::SetUp();
    InitPowerButtonControllerMembers(
        chromeos::PowerManagerClient::TabletMode::ON);
    InitScreenshotTestApi();
    screenshot_delegate_ = GetScreenshotDelegate();
    EnableTabletMode(true);

    // Advance a duration longer than |kIgnorePowerButtonAfterResumeDelay| to
    // avoid events being ignored.
    tick_clock_->Advance(
        TabletPowerButtonController::kIgnorePowerButtonAfterResumeDelay +
        base::TimeDelta::FromMilliseconds(2));
  }

 protected:
  // PowerButtonTestBase:
  void PressKey(ui::KeyboardCode key_code) override {
    last_key_event_ = std::make_unique<ui::KeyEvent>(ui::ET_KEY_PRESSED,
                                                     key_code, ui::EF_NONE);
    screenshot_controller_->OnKeyEvent(last_key_event_.get());
  }
  void ReleaseKey(ui::KeyboardCode key_code) override {
    last_key_event_ = std::make_unique<ui::KeyEvent>(ui::ET_KEY_RELEASED,
                                                     key_code, ui::EF_NONE);
    screenshot_controller_->OnKeyEvent(last_key_event_.get());
  }

  void InitScreenshotTestApi() {
    screenshot_test_api_ =
        std::make_unique<PowerButtonScreenshotControllerTestApi>(
            screenshot_controller_);
  }

  int GetScreenshotCount() const {
    return screenshot_delegate_->handle_take_screenshot_count();
  }

  void ResetScreenshotCount() {
    return screenshot_delegate_->reset_handle_take_screenshot_count();
  }

  bool LastKeyConsumed() const {
    DCHECK(last_key_event_);
    return last_key_event_->stopped_propagation();
  }

  TestScreenshotDelegate* screenshot_delegate_ = nullptr;  // Not owned.
  std::unique_ptr<PowerButtonScreenshotControllerTestApi> screenshot_test_api_;

  // Stores the last key event. Can be NULL if not set through PressKey() or
  // ReleaseKey().
  std::unique_ptr<ui::KeyEvent> last_key_event_;

  DISALLOW_COPY_AND_ASSIGN(PowerButtonScreenshotControllerTest);
};

// Tests power button screenshot accelerator works on tablet mode only.
TEST_F(PowerButtonScreenshotControllerTest, TabletMode) {
  // Tests on tablet mode pressing power button and volume down simultaneously
  // takes a screenshot.
  PressKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  ReleasePowerButton();
  EXPECT_EQ(1, GetScreenshotCount());

  // Tests screenshot handling is not active when not on tablet mode.
  ResetScreenshotCount();
  EnableTabletMode(false);
  PressKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  ReleasePowerButton();
  EXPECT_EQ(0, GetScreenshotCount());
}

// Tests if power-button/volume-down is released before volume-down/power-button
// is pressed, it does not take screenshot.
TEST_F(PowerButtonScreenshotControllerTest, ReleaseBeforeAnotherPressed) {
  // Releases volume down before power button is pressed.
  PressKey(ui::VKEY_VOLUME_DOWN);
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  ReleasePowerButton();
  EXPECT_EQ(0, GetScreenshotCount());

  // Releases power button before volume down is pressed.
  PressPowerButton();
  ReleasePowerButton();
  PressKey(ui::VKEY_VOLUME_DOWN);
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
}

// Tests power button is pressed first and meets screenshot chord condition.
TEST_F(PowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_ScreenshotChord) {
  PressPowerButton();
  tick_clock_->Advance(PowerButtonScreenshotController::kScreenshotChordDelay -
                       base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  // Verifies screenshot is taken, volume down is consumed.
  EXPECT_EQ(1, GetScreenshotCount());
  EXPECT_TRUE(LastKeyConsumed());
  // Keeps pressing volume down key under screenshot chord condition will not
  // take screenshot again, volume down is also consumed.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(1));
  ResetScreenshotCount();
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_TRUE(LastKeyConsumed());
  // Keeps pressing volume down key off screenshot chord condition will not
  // take screenshot and still consume volume down event.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_TRUE(LastKeyConsumed());

  // Releases power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Releases volume down key, and verifies nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
}

// Tests power button is pressed first, and then volume down pressed but doesn't
// meet screenshot chord condition.
TEST_F(PowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_NoScreenshotChord) {
  PressPowerButton();
  tick_clock_->Advance(PowerButtonScreenshotController::kScreenshotChordDelay +
                       base::TimeDelta::FromMilliseconds(1));
  PressKey(ui::VKEY_VOLUME_DOWN);
  // Verifies screenshot is not taken, volume down is not consumed.
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
  // Keeps pressing volume down key should continue triggerring volume down.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
  // Releases power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Releases volume down key, and verifies nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
}

// Tests volume key pressed cancels the ongoing convertible power button.
TEST_F(PowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_VolumeKeyCancelConvertiblePowerButton) {
  // Tests volume down key can stop convertible power button's shutdown timer.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->ShutdownTimerIsRunning());
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(tablet_test_api_->ShutdownTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());

  // Tests volume down key can stop shutdown animation timer.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->TriggerShutdownTimeout());
  EXPECT_TRUE(lock_state_test_api_->shutdown_timer_is_running());
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(lock_state_test_api_->shutdown_timer_is_running());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());

  // Tests volume up key can stop convertible power button's shutdown timer.
  // Also tests that volume up key is not consumed.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->ShutdownTimerIsRunning());
  PressKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(LastKeyConsumed());
  EXPECT_FALSE(tablet_test_api_->ShutdownTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  EXPECT_FALSE(LastKeyConsumed());

  // Tests volume up key can stop shutdown animation timer.
  // Also tests that volume up key is not consumed.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->TriggerShutdownTimeout());
  EXPECT_TRUE(lock_state_test_api_->shutdown_timer_is_running());
  PressKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(LastKeyConsumed());
  EXPECT_FALSE(lock_state_test_api_->shutdown_timer_is_running());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  EXPECT_FALSE(LastKeyConsumed());
}

// Tests volume down key pressed first and meets screenshot chord condition.
TEST_F(PowerButtonScreenshotControllerTest,
       VolumeDownPressedFirst_ScreenshotChord) {
  // Tests when volume down pressed first, it waits for power button pressed
  // screenshot chord.
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_TRUE(LastKeyConsumed());
  // Presses power button under screenshot chord condition, and verifies that
  // screenshot is taken.
  tick_clock_->Advance(PowerButtonScreenshotController::kScreenshotChordDelay -
                       base::TimeDelta::FromMilliseconds(2));
  PressPowerButton();
  EXPECT_EQ(1, GetScreenshotCount());
  // Keeps pressing volume down key under screenshot chord condition will not
  // take screenshot again, volume down is also consumed.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(1));
  ResetScreenshotCount();
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_TRUE(LastKeyConsumed());
  // Keeps pressing volume down key off screenshot chord condition will not take
  // screenshot and still consume volume down event.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_TRUE(LastKeyConsumed());

  // Releases power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Releases volume down key, and verifies nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
}

// Tests volume down key pressed first, and then power button pressed but
// doesn't meet screenshot chord condition.
TEST_F(PowerButtonScreenshotControllerTest,
       VolumeDownPressedFirst_NoScreenshotChord) {
  // Tests when volume down pressed first, it waits for power button pressed
  // screenshot chord.
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_TRUE(LastKeyConsumed());
  // Advances |tick_clock_| to off screenshot chord point. This will also
  // trigger volume down timer timeout, which will perform a volume down
  // operation.
  tick_clock_->Advance(PowerButtonScreenshotController::kScreenshotChordDelay +
                       base::TimeDelta::FromMilliseconds(1));
  EXPECT_TRUE(screenshot_test_api_->TriggerVolumeDownTimer());
  // Presses power button would not take screenshot.
  PressPowerButton();
  EXPECT_EQ(0, GetScreenshotCount());
  // Keeps pressing volume down key should continue triggerring volume down.
  tick_clock_->Advance(base::TimeDelta::FromMilliseconds(2));
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
  // Releases power button now should not set display off.
  ReleasePowerButton();
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  // Releases volume down key, and verifies nothing happens.
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
}

// Tests volume key pressed first invalidates convertible power button behavior.
TEST_F(PowerButtonScreenshotControllerTest,
       VolumeKeyPressedFirst_InvalidateConvertiblePowerButton) {
  // Tests volume down key invalidates convertible power button behavior.
  PressKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  EXPECT_FALSE(tablet_test_api_->ShutdownTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());

  // Tests volume up key invalidates convertible power button behavior. Also
  // tests that volume up key is not consumed.
  PressKey(ui::VKEY_VOLUME_UP);
  PressPowerButton();
  EXPECT_FALSE(LastKeyConsumed());
  EXPECT_FALSE(tablet_test_api_->ShutdownTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  EXPECT_FALSE(LastKeyConsumed());
}

// Test fixture used for testing power button screenshot behavior under forced
// clamshell power button.
class ClamshellPowerButtonScreenshotControllerTest
    : public PowerButtonScreenshotControllerTest {
 public:
  ClamshellPowerButtonScreenshotControllerTest() = default;
  ~ClamshellPowerButtonScreenshotControllerTest() override = default;

  // PowerButtonScreenshotControllerTest:
  void SetUp() override {
    PowerButtonScreenshotControllerTest::SetUp();
    ForceClamshellPowerButton();
    SetTabletModeSwitchState(chromeos::PowerManagerClient::TabletMode::ON);
    InitScreenshotTestApi();
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ClamshellPowerButtonScreenshotControllerTest);
};

// Tests on user session power button pressed first for waiting volume down key,
// and it timeouts.
TEST_F(ClamshellPowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_UserSessionWaitForVolumeDownKey) {
  Initialize(ButtonType::NORMAL, LoginStatus::USER);
  PressPowerButton();
  // Verifies that lock animation is not running when waiting for volume key.
  EXPECT_TRUE(screenshot_test_api_->ClamshellPowerButtonTimerIsRunning());
  EXPECT_FALSE(lock_state_test_api_->is_animating_lock());
  // Starts lock animation once it is done with waiting.
  EXPECT_TRUE(screenshot_test_api_->TriggerClamshellPowerButtonTimer());
  EXPECT_TRUE(lock_state_test_api_->is_animating_lock());
  ReleasePowerButton();
}

// Tests on non user session power button pressed first for waiting volume down
// key, and it timeouts.
TEST_F(ClamshellPowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_NonUserSessionWaitForVolumeDownKey) {
  Initialize(ButtonType::NORMAL, LoginStatus::NOT_LOGGED_IN);
  PressPowerButton();
  // Verifies that shutdown animation is not running when waiting for volume
  // key.
  EXPECT_TRUE(screenshot_test_api_->ClamshellPowerButtonTimerIsRunning());
  EXPECT_FALSE(lock_state_test_api_->shutdown_timer_is_running());
  // Starts shutdown animation once it is done with waiting.
  EXPECT_TRUE(screenshot_test_api_->TriggerClamshellPowerButtonTimer());
  EXPECT_TRUE(lock_state_test_api_->shutdown_timer_is_running());
  ReleasePowerButton();
}

// Tests when power button pressed first, its release or volume key pressed
// should stop clamshell power button timer to invalidate the delayed power
// button behavior.
TEST_F(ClamshellPowerButtonScreenshotControllerTest,
       PowerButtonPressedFirst_StopClamshellPowerButtonTimer) {
  Initialize(ButtonType::NORMAL, LoginStatus::USER);
  // Tests volume down key stops clamshell power button timer.
  PressPowerButton();
  EXPECT_TRUE(screenshot_test_api_->ClamshellPowerButtonTimerIsRunning());
  tick_clock_->Advance(PowerButtonScreenshotController::kScreenshotChordDelay -
                       base::TimeDelta::FromMilliseconds(1));
  PressKey(ui::VKEY_VOLUME_DOWN);
  // Under screenshot chord condition, screenshot is taken and volume down key
  // pressed is consumed.
  EXPECT_EQ(1, GetScreenshotCount());
  EXPECT_TRUE(LastKeyConsumed());
  EXPECT_FALSE(screenshot_test_api_->ClamshellPowerButtonTimerIsRunning());
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  ReleasePowerButton();
  EXPECT_FALSE(LastKeyConsumed());

  // Tests volume up key stops clamshell power button timer.
  ResetScreenshotCount();
  PressPowerButton();
  EXPECT_TRUE(screenshot_test_api_->ClamshellPowerButtonTimerIsRunning());
  PressKey(ui::VKEY_VOLUME_UP);
  // volume up is not used for taking screenshot so it is not consumed.
  EXPECT_EQ(0, GetScreenshotCount());
  EXPECT_FALSE(LastKeyConsumed());
  EXPECT_FALSE(screenshot_test_api_->ClamshellPowerButtonTimerIsRunning());
  ReleaseKey(ui::VKEY_VOLUME_UP);
  ReleasePowerButton();
  EXPECT_FALSE(LastKeyConsumed());

  // Tests power button release before its timeout stops clamshell power button
  // timer.
  PressPowerButton();
  EXPECT_TRUE(screenshot_test_api_->ClamshellPowerButtonTimerIsRunning());
  ReleasePowerButton();
  EXPECT_FALSE(screenshot_test_api_->ClamshellPowerButtonTimerIsRunning());
}

// Same as PowerButtonTestBase except that switches::kShowPowerButtonMenu is
// set.
class PowerButtonScreenshotControllerShowMenuTest
    : public PowerButtonScreenshotControllerTest {
 public:
  PowerButtonScreenshotControllerShowMenuTest() {
    set_show_power_button_menu(true);
  }
  ~PowerButtonScreenshotControllerShowMenuTest() override = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(PowerButtonScreenshotControllerShowMenuTest);
};

// Tests volume key pressed stops showing the power button menu.
TEST_F(PowerButtonScreenshotControllerShowMenuTest,
       PowerButtonPressedFirst_VolumeKeyCancelTabletPowerButton) {
  // Tests volume down key can stop the power button menu timer.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->PowerButtonMenuTimerIsRunning());
  PressKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(tablet_test_api_->PowerButtonMenuTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());

  // Tests volume up key can stop power button menu timer. Also tests that
  // volume up key is not consumed.
  PressPowerButton();
  EXPECT_TRUE(tablet_test_api_->PowerButtonMenuTimerIsRunning());
  PressKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(LastKeyConsumed());
  EXPECT_FALSE(tablet_test_api_->PowerButtonMenuTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  EXPECT_FALSE(LastKeyConsumed());
}

// Tests volume key pressed first invalidates showing the power button menu.
TEST_F(PowerButtonScreenshotControllerShowMenuTest,
       VolumeKeyPressedFirst_InvalidateTabletPowerButton) {
  // Tests volume down key invalidates showing the power button menu.
  PressKey(ui::VKEY_VOLUME_DOWN);
  PressPowerButton();
  EXPECT_FALSE(tablet_test_api_->PowerButtonMenuTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_DOWN);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());

  // Tests volume up key invalidates showing the power button menu. Also tests
  // that volume up key is not consumed.
  PressKey(ui::VKEY_VOLUME_UP);
  PressPowerButton();
  EXPECT_FALSE(LastKeyConsumed());
  EXPECT_FALSE(tablet_test_api_->PowerButtonMenuTimerIsRunning());
  ReleasePowerButton();
  ReleaseKey(ui::VKEY_VOLUME_UP);
  EXPECT_FALSE(power_manager_client_->backlights_forced_off());
  EXPECT_FALSE(LastKeyConsumed());
}

}  // namespace ash
