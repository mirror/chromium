// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/lock_screen_note_controller.h"

#include "ash/lock_screen_action/lock_screen_note_launcher.h"
#include "ash/public/interfaces/tray_action.mojom.h"
#include "ash/shell.h"
#include "ash/tray_action/tray_action.h"

namespace ash {

LockScreenNoteController::LockScreenNoteController() = default;

LockScreenNoteController::~LockScreenNoteController() = default;

void LockScreenNoteController::HandleStylusEject() {
  if (!LockScreenNoteLauncher::CanAttemptLaunch())
    return;

  DCHECK(!lock_screen_note_launcher_);

  lock_screen_note_launcher_ = std::make_unique<LockScreenNoteLauncher>();
  lock_screen_note_launcher_->Run(
      mojom::LockScreenNoteOrigin::kStylusEject,
      base::Bind(&LockScreenNoteController::NoteLaunchDone,
                 base::Unretained(this)));
}

bool LockScreenNoteController::HandleDisplayForcedOff(
    bool forced_off,
    base::OnceClosure callback) {
  bool should_delay_update = !forced_off && lock_screen_note_launcher_;
  delayed_forced_off_callback_ =
      should_delay_update ? std::move(callback) : base::OnceClosure();

  if (forced_off) {
    lock_screen_note_launcher_.reset();
    Shell::Get()->tray_action()->CloseLockScreenNote(
        mojom::CloseLockScreenNoteReason::kScreenDimmed);
  }

  return should_delay_update;
}

void LockScreenNoteController::NoteLaunchDone(bool success) {
  lock_screen_note_launcher_.reset();

  if (delayed_forced_off_callback_)
    std::move(delayed_forced_off_callback_).Run();
}

}  // namespace ash
