// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/lock_screen_note_controller.h"

#include <utility>

#include "ash/lock_screen_action/lock_screen_note_launcher.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ash/shell.h"
#include "ash/tray_action/tray_action.h"
#include "base/bind.h"
#include "base/location.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"

namespace ash {

namespace {

// The max amount of time display state change handling can be delayed due to a
// lock screen note action launch. The time starts running when the app launch
// is requested.
constexpr int kNoteLaunchTimeoutMs = 1500;

}  // namespace

LockScreenNoteController::LockScreenNoteController(base::TickClock* tick_clock)
    : launch_timer_(tick_clock), weak_ptr_factory_(this) {}

LockScreenNoteController::~LockScreenNoteController() = default;

void LockScreenNoteController::AttemptNoteLaunchForStylusEject() {
  if (!LockScreenNoteLauncher::CanAttemptLaunch() || lock_screen_note_launcher_)
    return;

  DCHECK(!launch_timer_.IsRunning());
  launch_timer_.Start(FROM_HERE,
                      base::TimeDelta::FromMilliseconds(kNoteLaunchTimeoutMs),
                      base::Bind(&LockScreenNoteController::NoteLaunchDone,
                                 weak_ptr_factory_.GetWeakPtr(), false));

  lock_screen_note_launcher_ = std::make_unique<LockScreenNoteLauncher>();
  lock_screen_note_launcher_->Run(
      mojom::LockScreenNoteOrigin::kStylusEject,
      base::Bind(&LockScreenNoteController::NoteLaunchDone,
                 weak_ptr_factory_.GetWeakPtr()));
}

bool LockScreenNoteController::HandleDisplayForcedOff(
    bool forced_off,
    base::OnceClosure callback) {
  bool wait_for_note_launch = !forced_off && lock_screen_note_launcher_;
  delayed_forced_off_callback_ =
      wait_for_note_launch ? std::move(callback) : base::OnceClosure();

  if (forced_off) {
    launch_timer_.Stop();
    lock_screen_note_launcher_.reset();

    Shell::Get()->tray_action()->CloseLockScreenNote(
        mojom::CloseLockScreenNoteReason::kScreenDimmed);
  }

  return !wait_for_note_launch;
}

void LockScreenNoteController::NoteLaunchDone(bool success) {
  lock_screen_note_launcher_.reset();
  launch_timer_.Stop();

  if (delayed_forced_off_callback_)
    std::move(delayed_forced_off_callback_).Run();
}

}  // namespace ash
