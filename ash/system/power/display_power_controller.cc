// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/display_power_controller.h"

#include <vector>

#include "ash/accessibility/accessibility_delegate.h"
#include "ash/media_controller.h"
#include "ash/public/cpp/ash_switches.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ash/shell.h"
#include "ash/system/power/display_power_controller_observer.h"
#include "ash/system/power/scoped_display_forced_off.h"
#include "ash/touch/touch_devices_controller.h"
#include "ash/tray_action/tray_action.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "ui/events/devices/input_device_manager.h"
#include "ui/events/devices/stylus_state.h"
#include "ui/events/event.h"

namespace ash {

namespace {

// Returns true if device is currently in tablet/tablet mode, otherwise false.
bool IsTabletModeActive() {
  TabletModeController* tablet_mode_controller =
      Shell::Get()->tablet_mode_controller();
  return tablet_mode_controller &&
         tablet_mode_controller->IsTabletModeWindowManagerEnabled();
}

}  // namespace

DisplayPowerController::DisplayPowerController() : weak_ptr_factory_(this) {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(
      this);
  ui::InputDeviceManager::GetInstance()->AddObserver(this);
  Shell::Get()->PrependPreTargetHandler(this);

  disable_touchscreen_while_screen_off_ =
      !base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kTouchscreenUsableWhileScreenOff);

  GetInitialBacklightsForcedOff();
}

DisplayPowerController::~DisplayPowerController() {
  Shell::Get()->RemovePreTargetHandler(this);
  ui::InputDeviceManager::GetInstance()->RemoveObserver(this);
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(
      this);
}

void DisplayPowerController::AddObserver(
    DisplayPowerControllerObserver* observer) {
  observers_.AddObserver(observer);
}

void DisplayPowerController::RemoveObserver(
    DisplayPowerControllerObserver* observer) {
  observers_.RemoveObserver(observer);
}

std::unique_ptr<ScopedDisplayForcedOff>
DisplayPowerController::ForceDisplayOff() {
  auto scoped_display_forced_off = std::make_unique<ScopedDisplayForcedOff>(
      base::BindOnce(&DisplayPowerController::UnregisterDisplayForcedOff,
                     weak_ptr_factory_.GetWeakPtr()));
  active_display_forced_offs_.insert(scoped_display_forced_off.get());
  SetDisplayForcedOff(true);
  return scoped_display_forced_off;
}

void DisplayPowerController::UnregisterDisplayForcedOff(
    ScopedDisplayForcedOff* scoped_display_forced_off) {
  active_display_forced_offs_.erase(scoped_display_forced_off);
  if (active_display_forced_offs_.empty())
    SetDisplayForcedOff(false);
}

void DisplayPowerController::CancelDisplayForcedOffForUserAction() {
  std::vector<ScopedDisplayForcedOff*> to_cancel;
  for (ScopedDisplayForcedOff* forced_off : active_display_forced_offs_) {
    if (forced_off->canceled_by_user_action())
      to_cancel.push_back(forced_off);
  }

  for (auto* forced_off : to_cancel)
    forced_off->Reset();
}

void DisplayPowerController::SetDisplayForcedOff(bool forced_off) {
  if (backlights_forced_off_ == forced_off)
    return;

  // Set the display and keyboard backlights (if present) to |forced_off|.
  chromeos::DBusThreadManager::Get()
      ->GetPowerManagerClient()
      ->SetBacklightsForcedOff(forced_off);
  backlights_forced_off_ = forced_off;
  UpdateTouchscreenStatus();

  if (backlights_forced_off_)
    Shell::Get()->media_controller()->SuspendMediaSessions();

  // Send an a11y alert.
  Shell::Get()->accessibility_delegate()->TriggerAccessibilityAlert(
      forced_off ? A11Y_ALERT_SCREEN_OFF : A11Y_ALERT_SCREEN_ON);

  for (auto& observer : observers_)
    observer.OnBacklightsForcedOffChanged();
}

void DisplayPowerController::PowerManagerRestarted() {
  chromeos::DBusThreadManager::Get()
      ->GetPowerManagerClient()
      ->SetBacklightsForcedOff(backlights_forced_off_);
}

void DisplayPowerController::BrightnessChanged(int level, bool user_initiated) {
  const ScreenState old_state = screen_state_;
  if (level != 0)
    screen_state_ = ScreenState::ON;
  else
    screen_state_ = user_initiated ? ScreenState::OFF : ScreenState::OFF_AUTO;

  if (screen_state_ != old_state) {
    for (auto& observer : observers_)
      observer.OnScreenStateChanged();
  }

  // Disable the touchscreen when the screen is turned off due to inactivity:
  // https://crbug.com/743291
  if ((screen_state_ == ScreenState::OFF_AUTO) !=
          (old_state == ScreenState::OFF_AUTO) &&
      disable_touchscreen_while_screen_off_)
    UpdateTouchscreenStatus();
}

void DisplayPowerController::SuspendDone(
    const base::TimeDelta& sleep_duration) {
  // Stop forcing backlights off on resume to handle situations where the power
  // button resumed but we didn't receive the event (crbug.com/735291).
  CancelDisplayForcedOffForUserAction();
}

void DisplayPowerController::LidEventReceived(
    chromeos::PowerManagerClient::LidState state,
    const base::TimeTicks& timestamp) {
  CancelDisplayForcedOffForUserAction();
}

void DisplayPowerController::OnKeyEvent(ui::KeyEvent* event) {
  // Ignore key events generated by the power button since power button activity
  // is already handled elsewhere.
  if (event->key_code() == ui::VKEY_POWER)
    return;

  if (!IsTabletModeActive())
    CancelDisplayForcedOffForUserAction();
}

void DisplayPowerController::OnMouseEvent(ui::MouseEvent* event) {
  if (event->flags() & ui::EF_IS_SYNTHESIZED)
    return;

  if (!IsTabletModeActive())
    CancelDisplayForcedOffForUserAction();
}

void DisplayPowerController::OnStylusStateChanged(ui::StylusState state) {
  if (state == ui::StylusState::REMOVED) {
    for (auto& observer : observers_)
      observer.OnWillHandleStylusRemoved();

    CancelDisplayForcedOffForUserAction();
  }
}

void DisplayPowerController::OnPowerButtonEvent(
    bool down,
    const base::TimeTicks& timestamp) {
  if (down)
    CancelDisplayForcedOffForUserAction();
}

void DisplayPowerController::GetInitialBacklightsForcedOff() {
  chromeos::DBusThreadManager::Get()
      ->GetPowerManagerClient()
      ->GetBacklightsForcedOff(base::BindOnce(
          &DisplayPowerController::OnGotInitialBacklightsForcedOff,
          weak_ptr_factory_.GetWeakPtr()));
}

void DisplayPowerController::OnGotInitialBacklightsForcedOff(
    base::Optional<bool> is_forced_off) {
  backlights_forced_off_ = is_forced_off.value_or(false);
  UpdateTouchscreenStatus();
}

void DisplayPowerController::UpdateTouchscreenStatus() {
  const bool disable_touchscreen =
      backlights_forced_off_ || (screen_state_ == ScreenState::OFF_AUTO &&
                                 disable_touchscreen_while_screen_off_);
  Shell::Get()->touch_devices_controller()->SetTouchscreenEnabled(
      !disable_touchscreen, TouchscreenEnabledSource::GLOBAL);
}

}  // namespace ash
