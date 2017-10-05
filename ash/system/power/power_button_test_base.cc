// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_button_test_base.h"

#include "ash/public/cpp/ash_switches.h"
#include "ash/public/cpp/config.h"
#include "ash/shell.h"
#include "ash/shell_test_api.h"
#include "ash/wm/lock_state_controller.h"
#include "ash/wm/lock_state_controller_test_api.h"
#include "ash/wm/power_button_controller.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/command_line.h"
#include "base/test/simple_test_tick_clock.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/fake_power_manager_client.h"
#include "ui/events/event.h"
#include "ui/events/test/event_generator.h"

namespace ash {

constexpr gfx::Vector3dF PowerButtonTestBase::kUpVector;

constexpr gfx::Vector3dF PowerButtonTestBase::kDownVector;

constexpr gfx::Vector3dF PowerButtonTestBase::kSidewaysVector;

PowerButtonTestBase::PowerButtonTestBase() = default;

PowerButtonTestBase::~PowerButtonTestBase() = default;

void PowerButtonTestBase::SetUp() {
  // This also initializes DBusThreadManager.
  std::unique_ptr<chromeos::DBusThreadManagerSetter> dbus_setter =
      chromeos::DBusThreadManager::GetSetterForTesting();
  power_manager_client_ = new chromeos::FakePowerManagerClient();
  dbus_setter->SetPowerManagerClient(base::WrapUnique(power_manager_client_));
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kAshEnableTabletMode);
  AshTestBase::SetUp();

  lock_state_controller_ = Shell::Get()->lock_state_controller();
  lock_state_test_api_ =
      std::make_unique<LockStateControllerTestApi>(lock_state_controller_);
}

void PowerButtonTestBase::TearDown() {
  const Config config = Shell::GetAshConfig();
  AshTestBase::TearDown();
  // Mash/mus shuts down dbus after each test.
  if (config == Config::CLASSIC)
    chromeos::DBusThreadManager::Shutdown();
}

void PowerButtonTestBase::ResetPowerButtonController() {
  ShellTestApi().ResetPowerButtonControllerForTest();
  InitPowerButtonControllerMembers(false /* send_accelerometer_update */);
}

void PowerButtonTestBase::InitPowerButtonControllerMembers(
    bool send_accelerometer_update) {
  power_button_controller_ = Shell::Get()->power_button_controller();
  tick_clock_ = new base::SimpleTestTickClock;
  power_button_controller_->SetTickClockForTesting(
      base::WrapUnique(tick_clock_));

  if (send_accelerometer_update) {
    SendAccelerometerUpdate(kSidewaysVector, kUpVector);
    tablet_controller_ =
        power_button_controller_->tablet_power_button_controller_for_test();
    tablet_test_api_ = std::make_unique<TabletPowerButtonController::TestApi>(
        tablet_controller_);
    screenshot_controller_ =
        power_button_controller_->power_button_screenshot_controller_for_test();
  } else {
    tablet_test_api_ = nullptr;
    tablet_controller_ = nullptr;
    screenshot_controller_ = nullptr;
  }
}

void PowerButtonTestBase::SendAccelerometerUpdate(
    const gfx::Vector3dF& screen,
    const gfx::Vector3dF& keyboard) {
  scoped_refptr<chromeos::AccelerometerUpdate> update(
      new chromeos::AccelerometerUpdate());
  update->Set(chromeos::ACCELEROMETER_SOURCE_SCREEN, screen.x(), screen.y(),
              screen.z());
  update->Set(chromeos::ACCELEROMETER_SOURCE_ATTACHED_KEYBOARD, keyboard.x(),
              keyboard.y(), keyboard.z());

  power_button_controller_->OnAccelerometerUpdated(update);
  tablet_controller_ =
      power_button_controller_->tablet_power_button_controller_for_test();
  screenshot_controller_ =
      power_button_controller_->power_button_screenshot_controller_for_test();

  if (tablet_test_api_ && tablet_test_api_->IsObservingAccelerometerReader(
                              chromeos::AccelerometerReader::GetInstance()))
    tablet_controller_->OnAccelerometerUpdated(update);
}

void PowerButtonTestBase::ForceClamshellPowerButton() {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kForceClamshellPowerButton);
  ResetPowerButtonController();
}

void PowerButtonTestBase::PressPowerButton() {
  power_button_controller_->PowerButtonEventReceived(true,
                                                     tick_clock_->NowTicks());
}

void PowerButtonTestBase::ReleasePowerButton() {
  power_button_controller_->PowerButtonEventReceived(false,
                                                     tick_clock_->NowTicks());
}

void PowerButtonTestBase::PressKey(ui::KeyboardCode key_code) {
  GetEventGenerator().PressKey(key_code, ui::EF_NONE);
}

void PowerButtonTestBase::ReleaseKey(ui::KeyboardCode key_code) {
  GetEventGenerator().ReleaseKey(key_code, ui::EF_NONE);
}

void PowerButtonTestBase::Initialize(LoginStatus status) {
  if (status == LoginStatus::NOT_LOGGED_IN) {
    ClearLogin();
  } else {
    CreateUserSessions(1);
  }
}

void PowerButtonTestBase::EnableTabletMode(bool enable) {
  Shell::Get()->tablet_mode_controller()->EnableTabletModeWindowManager(enable);
}

}  // namespace ash
