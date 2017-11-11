// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/lock_screen_action/lock_screen_note_display_handler.h"

#include <utility>

#include "ash/lock_screen_action/lock_screen_note_launcher.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ash/shell.h"
#include "ash/system/power/display_power_controller.h"
#include "ash/system/power/scoped_display_forced_off.h"
#include "ash/tray_action/tray_action.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/time/time.h"
#include "ui/events/devices/input_device_manager.h"
#include "ui/events/devices/stylus_state.h"

namespace ash {

namespace {

// The max amount of time display state change handling can be delayed due to a
// lock screen note action launch. The time starts running when the app launch
// is requested.
constexpr int kNoteLaunchTimeoutMs = 1500;

}  // namespace

LockScreenNoteDisplayHandler::LockScreenNoteDisplayHandler(
    DisplayPowerController* display_controller)
    : display_controller_(display_controller),
      stylus_state_observer_(this),
      display_observer_(this),
      weak_ptr_factory_(this) {
  stylus_state_observer_.Add(ui::InputDeviceManager::GetInstance());
  display_observer_.Add(display_controller_);
}

LockScreenNoteDisplayHandler::~LockScreenNoteDisplayHandler() = default;

void LockScreenNoteDisplayHandler::OnScreenStateChanged() {
  if (display_controller_->screen_state() !=
          DisplayPowerController::ScreenState::ON &&
      note_launch_delayed_until_screen_off_) {
    RunLockScreenNoteLauncher();
  }
}

void LockScreenNoteDisplayHandler::OnBacklightsForcedOffChanged() {
  if (display_controller_->backlights_forced_off()) {
    Shell::Get()->tray_action()->CloseLockScreenNote(
        mojom::CloseLockScreenNoteReason::kScreenDimmed);
  } else if (note_launch_delayed_until_screen_off_) {
    RunLockScreenNoteLauncher();
  }
}

void LockScreenNoteDisplayHandler::OnWillHandleStylusRemoved() {
  if (!LockScreenNoteLauncher::CanAttemptLaunch())
    return;

  // Preemptively set display state to forced off (if it already is not in this
  // state). This is expected to get called when stylus is removed, an event
  // which will cause this object to start a lock screen note launch, at which
  // point display will be forced off.
  // Display is forced off here as well to make sure DisplayPowerController
  // does not try to turn display back on in case it receives stylus removed
  // event before |this|.
  if (IsDisplayOff())
    ForceDisplayOff();
}

void LockScreenNoteDisplayHandler::OnStylusStateChanged(ui::StylusState state) {
  if (state == ui::StylusState::REMOVED)
    AttemptNoteLaunchForStylusEject();
}

void LockScreenNoteDisplayHandler::AttemptNoteLaunchForStylusEject() {
  if (!LockScreenNoteLauncher::CanAttemptLaunch() ||
      lock_screen_note_launcher_ || note_launch_delayed_until_screen_off_) {
    return;
  }

  // If display is currently off, force it off to ensure it does not get
  // turned off during note launch.
  if (IsDisplayOff())
    ForceDisplayOff();

  DCHECK(!launch_timer_.IsRunning());
  launch_timer_.Start(FROM_HERE,
                      base::TimeDelta::FromMilliseconds(kNoteLaunchTimeoutMs),
                      base::Bind(&LockScreenNoteDisplayHandler::NoteLaunchDone,
                                 weak_ptr_factory_.GetWeakPtr(), false));

  if (display_controller_->backlights_forced_off() &&
      display_controller_->screen_state() ==
          DisplayPowerController::ScreenState::ON) {
    note_launch_delayed_until_screen_off_ = true;
    return;
  }

  RunLockScreenNoteLauncher();
}

void LockScreenNoteDisplayHandler::RunLockScreenNoteLauncher() {
  DCHECK(!lock_screen_note_launcher_);

  note_launch_delayed_until_screen_off_ = false;

  lock_screen_note_launcher_ = std::make_unique<LockScreenNoteLauncher>();
  lock_screen_note_launcher_->Run(
      mojom::LockScreenNoteOrigin::kStylusEject,
      base::Bind(&LockScreenNoteDisplayHandler::NoteLaunchDone,
                 weak_ptr_factory_.GetWeakPtr()));
}

void LockScreenNoteDisplayHandler::ForceDisplayOff() {
  if (display_forced_off_)
    return;

  display_forced_off_ = display_controller_->ForceDisplayOff();
  display_forced_off_->set_canceled_by_user_action(false);
}

bool LockScreenNoteDisplayHandler::IsDisplayOff() {
  return display_controller_->backlights_forced_off() ||
         display_controller_->screen_state() !=
             DisplayPowerController::ScreenState::ON;
}

void LockScreenNoteDisplayHandler::NoteLaunchDone(bool success) {
  note_launch_delayed_until_screen_off_ = false;
  display_forced_off_.reset();
  lock_screen_note_launcher_.reset();
  launch_timer_.Stop();
}

}  // namespace ash
