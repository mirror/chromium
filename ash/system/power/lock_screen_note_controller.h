// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_LOCK_SCREEN_NOTE_CONTROLLER_H_
#define ASH_SYSTEM_POWER_LOCK_SCREEN_NOTE_CONTROLLER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"

namespace base {
class TickClock;
}

namespace ash {

class LockScreenNoteLauncher;

// Class used to control lock screen note state depending on display state set
// by power button display controller.
class LockScreenNoteController {
 public:
  explicit LockScreenNoteController(base::TickClock* tick_clock);
  ~LockScreenNoteController();

  // If lock screen note action is available, it requests a new lock screen note
  // with launch reason set to stylus eject.
  void AttemptNoteLaunchForStylusEject();

  // Updates the lock screen note state depending on whether display is forced
  // off.
  // It returns whether the new display state change has been handled
  // synchronously - in case this returns false, |callback| will be called when
  // the state change handling finishes (e. g. in case a note launch is pending
  // when display stops being forced off, the state change will be considered
  // handled when the note launch finishes, at which point |callback| will be
  // run).
  //
  // If called while previous state change is still pending, the callback
  // associated with the previous display state change will be discarded
  // (without being run).
  bool HandleDisplayForcedOff(bool forced_off, base::OnceClosure callback);

 private:
  friend class TabletPowerButtonControllerTestApi;

  void NoteLaunchDone(bool success);

  base::OnceClosure delayed_forced_off_callback_;
  std::unique_ptr<LockScreenNoteLauncher> lock_screen_note_launcher_;

  // Timer used to set up timeout for lock screen note launch.
  base::OneShotTimer launch_timer_;

  base::WeakPtrFactory<LockScreenNoteController> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenNoteController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_LOCK_SCREEN_NOTE_CONTROLLER_H_
