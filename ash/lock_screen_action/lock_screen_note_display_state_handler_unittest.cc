// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/lock_screen_action/lock_screen_note_display_state_handler.h"

#include <memory>
#include <utility>
#include <vector>

#include "ash/public/cpp/ash_switches.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ash/shell.h"
#include "ash/system/power/power_button_controller.h"
#include "ash/test/ash_test_base.h"
#include "ash/tray_action/test_tray_action_client.h"
#include "ash/tray_action/tray_action.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/test/simple_test_tick_clock.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "ui/events/devices/stylus_state.h"
#include "ui/events/test/device_data_manager_test_api.h"

namespace ash {

namespace {

constexpr double kVisibleBrightnessPercent = 10;

class TestPowerManagerClient : public chromeos::FakePowerManagerClient {
 public:
  TestPowerManagerClient() {
    set_screen_brightness_percent(kVisibleBrightnessPercent);
  }

  ~TestPowerManagerClient() override = default;

  void NotifyUserActivity(power_manager::UserActivityType type) override {
    if (!backlights_forced_off() && screen_brightness_percent() == 0) {
      SetScreenBrightnessForVisibility(true);
    }
  }

  void SetBacklightsForcedOff(bool forced_off) override {
    chromeos::FakePowerManagerClient::SetBacklightsForcedOff(forced_off);
    if (!delay_brightness_change_on_backlights_forced_off_)
      SetScreenBrightnessForVisibility(!forced_off);
  }

  void SetScreenBrightnessForVisibility(bool visible) {
    double target_brightness = visible ? kVisibleBrightnessPercent : 0;
    if (target_brightness == screen_brightness_percent())
      return;

    ++brightness_percent_changed_count_;
    set_screen_brightness_percent(target_brightness);
    SendBrightnessChanged(target_brightness, false);
    base::RunLoop().RunUntilIdle();
  }

  void UpdateBrightnessAfterBacklightsForcedOff() {
    ASSERT_TRUE(delay_brightness_change_on_backlights_forced_off_);
    ASSERT_NE(backlights_forced_off(), screen_brightness_percent() == 0);

    SetScreenBrightnessForVisibility(!backlights_forced_off());
  }

  void set_delay_brightness_change_on_backlights_forced_off() {
    delay_brightness_change_on_backlights_forced_off_ = true;
  }

  int brightness_percent_changed_count() {
    return brightness_percent_changed_count_;
  }

 private:
  int brightness_percent_changed_count_ = 0;
  bool delay_brightness_change_on_backlights_forced_off_ = false;

  DISALLOW_COPY_AND_ASSIGN(TestPowerManagerClient);
};

}  // namespace

class LockScreenNoteDisplayStateHandlerTest : public AshTestBase {
 public:
  LockScreenNoteDisplayStateHandlerTest() = default;
  ~LockScreenNoteDisplayStateHandlerTest() override = default;

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kAshEnableTabletMode);

    auto power_manager_client = std::make_unique<TestPowerManagerClient>();
    power_manager_client_ = power_manager_client.get();
    chromeos::DBusThreadManager::GetSetterForTesting()->SetPowerManagerClient(
        std::move(power_manager_client));

    AshTestBase::SetUp();

    BlockUserSession(BLOCKED_BY_LOCK_SCREEN);
    InitializeTabletPowerButtonState();

    Shell::Get()->tray_action()->SetClient(
        tray_action_client_.CreateInterfacePtrAndBind(),
        mojom::TrayActionState::kAvailable);
    Shell::Get()->tray_action()->FlushMojoForTesting();
    // Run the loop so the lock screen note display state handler picks up
    // initial screen brightness.
    base::RunLoop().RunUntilIdle();

    // Advance the tick clock so it's not close to the null clock value.
    tick_clock_.Advance(base::TimeDelta::FromMilliseconds(10000));
  }

  TestPowerManagerClient* power_manager_client() {
    return power_manager_client_;
  }

  bool LaunchTimeoutRunning() {
    base::OneShotTimer* timer =
        Shell::Get()
            ->tray_action()
            ->lock_screen_note_display_state_handler_for_test()
            ->launch_timer_for_test();
    return timer && timer->IsRunning();
  }

  bool TriggerNoteLaunchTimeout() {
    base::OneShotTimer* timer =
        Shell::Get()
            ->tray_action()
            ->lock_screen_note_display_state_handler_for_test()
            ->launch_timer_for_test();
    if (!timer || !timer->IsRunning())
      return false;

    base::Closure task = timer->user_task();
    timer->Stop();
    task.Run();
    return true;
  }

  void SimulatePowerButtonPress() {
    power_manager_client_->SendPowerButtonEvent(true, tick_clock_.NowTicks());
    tick_clock_.Advance(base::TimeDelta::FromMilliseconds(10));
    power_manager_client_->SendPowerButtonEvent(false, tick_clock_.NowTicks());
  }

  testing::AssertionResult SimulateNoteLaunchStartedIfNoteActionRequested(
      mojom::LockScreenNoteOrigin expected_note_origin) {
    Shell::Get()->tray_action()->FlushMojoForTesting();

    if (tray_action_client_.note_origins().size() != 1) {
      return testing::AssertionFailure()
             << "Expected a single note action request, found "
             << tray_action_client_.note_origins().size();
    }

    if (tray_action_client_.note_origins()[0] != expected_note_origin) {
      return testing::AssertionFailure()
             << "Unexpected note request origin: "
             << tray_action_client_.note_origins()[0]
             << "; expected: " << expected_note_origin;
    }

    Shell::Get()->tray_action()->UpdateLockScreenNoteState(
        mojom::TrayActionState::kLaunching);
    return testing::AssertionSuccess();
  }

  TestTrayActionClient* tray_action_client() { return &tray_action_client_; }

 private:
  void InitializeTabletPowerButtonState() {
    // Tablet button controller initialization is deffered until the
    // accelerometer update is received by the power button controller.
    scoped_refptr<chromeos::AccelerometerUpdate> accelerometer_update(
        new chromeos::AccelerometerUpdate());
    Shell::Get()->power_button_controller()->OnAccelerometerUpdated(
        accelerometer_update);
  }

  base::SimpleTestTickClock tick_clock_;
  TestPowerManagerClient* power_manager_client_ = nullptr;

  TestTrayActionClient tray_action_client_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenNoteDisplayStateHandlerTest);
};

TEST_F(LockScreenNoteDisplayStateHandlerTest, EjectWhenScreenOn) {
  ui::test::DeviceDataManagerTestAPI devices_test_api;
  devices_test_api.NotifyObserversStylusStateChanged(ui::StylusState::REMOVED);

  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(0, power_manager_client()->brightness_percent_changed_count());

  ASSERT_TRUE(SimulateNoteLaunchStartedIfNoteActionRequested(
      mojom::LockScreenNoteOrigin::kStylusEject));

  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kActive);

  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(0, power_manager_client()->brightness_percent_changed_count());

  ASSERT_FALSE(LaunchTimeoutRunning());
}

TEST_F(LockScreenNoteDisplayStateHandlerTest, EjectWhenScreenOff) {
  power_manager_client()->SetScreenBrightnessForVisibility(false);

  ui::test::DeviceDataManagerTestAPI devices_test_api;
  devices_test_api.NotifyObserversStylusStateChanged(ui::StylusState::REMOVED);

  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(1, power_manager_client()->brightness_percent_changed_count());

  ASSERT_TRUE(SimulateNoteLaunchStartedIfNoteActionRequested(
      mojom::LockScreenNoteOrigin::kStylusEject));

  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kActive);

  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(2, power_manager_client()->brightness_percent_changed_count());

  ASSERT_FALSE(LaunchTimeoutRunning());
}

TEST_F(LockScreenNoteDisplayStateHandlerTest,
       EjectWhenScreenOffAndNoteNotAvailable) {
  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kNotAvailable);

  power_manager_client()->SetScreenBrightnessForVisibility(false);
  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(1, power_manager_client()->brightness_percent_changed_count());

  ui::test::DeviceDataManagerTestAPI devices_test_api;
  devices_test_api.NotifyObserversStylusStateChanged(ui::StylusState::REMOVED);

  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(2, power_manager_client()->brightness_percent_changed_count());

  EXPECT_TRUE(tray_action_client()->note_origins().empty());

  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kLaunching);
  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kActive);
  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(2, power_manager_client()->brightness_percent_changed_count());

  ASSERT_FALSE(LaunchTimeoutRunning());
}

TEST_F(LockScreenNoteDisplayStateHandlerTest, TurnScreenOnWhenAppLaunchFails) {
  power_manager_client()->SetScreenBrightnessForVisibility(false);

  ui::test::DeviceDataManagerTestAPI devices_test_api;
  devices_test_api.NotifyObserversStylusStateChanged(ui::StylusState::REMOVED);

  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(1, power_manager_client()->brightness_percent_changed_count());

  ASSERT_TRUE(SimulateNoteLaunchStartedIfNoteActionRequested(
      mojom::LockScreenNoteOrigin::kStylusEject));

  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kAvailable);

  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(2, power_manager_client()->brightness_percent_changed_count());

  ASSERT_FALSE(LaunchTimeoutRunning());
}

// Tests stylus eject when the display was turned off by a power button getting
// pressed - in particular, it makes sure that power button display controller
// cancelling display forced off on stylus eject does not happen before lock
// screen note display state handler requests display to be forced off (i.e.
// that display is continuosly kept forced off).
TEST_F(LockScreenNoteDisplayStateHandlerTest, EjectWhileScreenForcedOff) {
  SimulatePowerButtonPress();

  ASSERT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(1, power_manager_client()->brightness_percent_changed_count());

  ui::test::DeviceDataManagerTestAPI devices_test_api;
  devices_test_api.NotifyObserversStylusStateChanged(ui::StylusState::REMOVED);

  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(1, power_manager_client()->brightness_percent_changed_count());

  ASSERT_TRUE(SimulateNoteLaunchStartedIfNoteActionRequested(
      mojom::LockScreenNoteOrigin::kStylusEject));

  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kActive);

  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(2, power_manager_client()->brightness_percent_changed_count());

  ASSERT_FALSE(LaunchTimeoutRunning());
}

TEST_F(LockScreenNoteDisplayStateHandlerTest, DisplayNotTurnedOffIndefinitely) {
  power_manager_client()->SetScreenBrightnessForVisibility(false);

  ui::test::DeviceDataManagerTestAPI devices_test_api;
  devices_test_api.NotifyObserversStylusStateChanged(ui::StylusState::REMOVED);

  ASSERT_TRUE(SimulateNoteLaunchStartedIfNoteActionRequested(
      mojom::LockScreenNoteOrigin::kStylusEject));

  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(1, power_manager_client()->brightness_percent_changed_count());

  ASSERT_TRUE(TriggerNoteLaunchTimeout());
  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(2, power_manager_client()->brightness_percent_changed_count());

  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kActive);
  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(2, power_manager_client()->brightness_percent_changed_count());

  ASSERT_FALSE(LaunchTimeoutRunning());
}

// Test that verifies that lock screen note request is delayed until screen is
// turned off if stylus eject happens soon after power button is pressed, while
// display configuration to off is still in progress.
TEST_F(LockScreenNoteDisplayStateHandlerTest,
       StylusEjectWhileForcingDisplayOff) {
  power_manager_client()
      ->set_delay_brightness_change_on_backlights_forced_off();

  SimulatePowerButtonPress();
  ASSERT_TRUE(power_manager_client()->backlights_forced_off());
  ASSERT_EQ(0, power_manager_client()->brightness_percent_changed_count());

  ui::test::DeviceDataManagerTestAPI devices_test_api;
  devices_test_api.NotifyObserversStylusStateChanged(ui::StylusState::REMOVED);

  Shell::Get()->tray_action()->FlushMojoForTesting();
  EXPECT_TRUE(tray_action_client()->note_origins().empty());

  power_manager_client()->UpdateBrightnessAfterBacklightsForcedOff();

  ASSERT_TRUE(SimulateNoteLaunchStartedIfNoteActionRequested(
      mojom::LockScreenNoteOrigin::kStylusEject));
  EXPECT_TRUE(power_manager_client()->backlights_forced_off());
  EXPECT_EQ(1, power_manager_client()->brightness_percent_changed_count());

  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kActive);
  EXPECT_FALSE(power_manager_client()->backlights_forced_off());
  power_manager_client()->UpdateBrightnessAfterBacklightsForcedOff();
  EXPECT_EQ(2, power_manager_client()->brightness_percent_changed_count());

  ASSERT_FALSE(LaunchTimeoutRunning());
}

TEST_F(LockScreenNoteDisplayStateHandlerTest,
       PowerButtonPressClosesLockScreenNote) {
  Shell::Get()->tray_action()->UpdateLockScreenNoteState(
      mojom::TrayActionState::kActive);

  SimulatePowerButtonPress();

  Shell::Get()->tray_action()->FlushMojoForTesting();
  EXPECT_EQ(std::vector<mojom::CloseLockScreenNoteReason>(
                {mojom::CloseLockScreenNoteReason::kScreenDimmed}),
            tray_action_client()->close_note_reasons());
}

}  // namespace ash
