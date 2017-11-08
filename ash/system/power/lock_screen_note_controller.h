// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_LOCK_SCREEN_NOTE_CONTROLLER_H_
#define ASH_SYSTEM_POWER_LOCK_SCREEN_NOTE_CONTROLLER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"

namespace ash {

class LockScreenNoteLauncher;

class LockScreenNoteController {
 public:
  LockScreenNoteController();
  ~LockScreenNoteController();

  void HandleStylusEject();
  bool HandleDisplayForcedOff(bool forced_off, base::OnceClosure callback);

 private:
  void NoteLaunchDone(bool success);

  base::OnceClosure delayed_forced_off_callback_;
  std::unique_ptr<LockScreenNoteLauncher> lock_screen_note_launcher_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenNoteController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_LOCK_SCREEN_NOTE_CONTROLLER_H_
