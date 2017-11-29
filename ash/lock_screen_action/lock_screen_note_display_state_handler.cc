// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/lock_screen_action/lock_screen_note_display_state_handler.h"

#include <utility>

#include "ash/lock_screen_action/lock_screen_note_launcher.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ash/shell.h"
#include "ash/system/power/scoped_display_forced_off.h"
#include "ash/tray_action/tray_action.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/time/time.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "ui/events/devices/input_device_manager.h"
#include "ui/events/devices/stylus_state.h"

namespace ash {

namespace {

// The max amount of time display state change handling can be delayed due to a
// lock screen note action launch. The time starts running when the app launch
// is requested.
constexpr int kNoteLaunchTimeoutMs = 1500;

}  // namespace

LockScreenNoteDisplayStateHandler::LockScreenNoteDisplayStateHandler(
    DisplayForcedOffSetter* display_forced_off_setter)
    : display_forced_off_setter_(display_forced_off_setter),
      display_forced_off_observer_(this),
      power_manager_observer_(this),
      weak_ptr_factory_(this) {
  display_forced_off_observer_.Add(display_forced_off_setter_);
  power_manager_observer_.Add(
      chromeos::DBusThreadManager::Get()->GetPowerManagerClient());

  chromeos::DBusThreadManager::Get()
      ->GetPowerManagerClient()
      ->GetScreenBrightnessPercent(base::Bind(
          &LockScreenNoteDisplayStateHandler::GotInitialScreenBrightness,
          weak_ptr_factory_.GetWeakPtr()));
}

LockScreenNoteDisplayStateHandler::~LockScreenNoteDisplayStateHandler() =
    default;

void LockScreenNoteDisplayStateHandler::OnBacklightsForcedOffChanged() {
  if (display_forced_off_setter_->backlights_forced_off()) {
    if (!display_forced_off_) {
      Shell::Get()->tray_action()->CloseLockScreenNote(
          mojom::CloseLockScreenNoteReason::kScreenDimmed);
    }
  } else if (note_launch_delayed_until_screen_off_) {
    RunLockScreenNoteLauncher();
  }
}

void LockScreenNoteDisplayStateHandler::BrightnessChanged(int level,
                                                          bool user_initiated) {
  screen_on_ = level > 0;

  if (!screen_on_.value() && note_launch_delayed_until_screen_off_)
    RunLockScreenNoteLauncher();
}

void LockScreenNoteDisplayStateHandler::AttemptNoteLaunchForStylusEject() {
  if (!LockScreenNoteLauncher::CanAttemptLaunch() || NoteLaunchInProgress())
    return;

  // If display is currently off, force it off to ensure it does not get
  // turned off during note launch.
  if (IsDisplayOff())
    ForceDisplayOff();

  DCHECK(!launch_timer_.IsRunning());
  launch_timer_.Start(
      FROM_HERE, base::TimeDelta::FromMilliseconds(kNoteLaunchTimeoutMs),
      base::Bind(&LockScreenNoteDisplayStateHandler::NoteLaunchDone,
                 weak_ptr_factory_.GetWeakPtr(), false));

  if (display_forced_off_setter_->backlights_forced_off() &&
      screen_on_.value_or(false)) {
    note_launch_delayed_until_screen_off_ = true;
    return;
  }

  RunLockScreenNoteLauncher();
}

void LockScreenNoteDisplayStateHandler::GotInitialScreenBrightness(
    base::Optional<double> brightness) {
  if (screen_on_.has_value() || !brightness.has_value())
    return;

  screen_on_ = brightness.value() > 0;
}

void LockScreenNoteDisplayStateHandler::RunLockScreenNoteLauncher() {
  DCHECK(!lock_screen_note_launcher_);

  note_launch_delayed_until_screen_off_ = false;

  lock_screen_note_launcher_ = std::make_unique<LockScreenNoteLauncher>();
  lock_screen_note_launcher_->Run(
      mojom::LockScreenNoteOrigin::kStylusEject,
      base::Bind(&LockScreenNoteDisplayStateHandler::NoteLaunchDone,
                 weak_ptr_factory_.GetWeakPtr()));
}

void LockScreenNoteDisplayStateHandler::ForceDisplayOff() {
  if (display_forced_off_ && display_forced_off_->IsActive())
    return;

  display_forced_off_ = display_forced_off_setter_->ForceDisplayOff();
}

bool LockScreenNoteDisplayStateHandler::IsDisplayOff() {
  return display_forced_off_setter_->backlights_forced_off() ||
         !screen_on_.value_or(true);
}

bool LockScreenNoteDisplayStateHandler::NoteLaunchInProgress() const {
  return note_launch_delayed_until_screen_off_ || lock_screen_note_launcher_;
}

void LockScreenNoteDisplayStateHandler::NoteLaunchDone(bool success) {
  note_launch_delayed_until_screen_off_ = false;
  display_forced_off_.reset();
  lock_screen_note_launcher_.reset();
  launch_timer_.Stop();
}

}  // namespace ash
