// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_NOTE_DISPLAY_STATE_HANDLER_H_
#define ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_NOTE_DISPLAY_STATE_HANDLER_H_

#include <memory>

#include "ash/system/power/display_forced_off_setter.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "base/timer/timer.h"
#include "chromeos/dbus/power_manager_client.h"

namespace ash {

class DisplayForcedOffSetter;
class LockScreenNoteLauncher;
class ScopedDisplayForcedOff;

// Handles display state changes related to lock screen note state.
// For example it will close any active lock screen notes if the display is
// forced off.
// This class also handles a lock screen note launch when stylus is ejected.
// When the note is launched while the screen is off, note launch forces the
// display off, in order to delay screen being turned on (which happens due to
// stylus eject) until the lock screen note is visible. This is to prevent
// flash of lock screen UI as the lock screen note app window is being shown.
class LockScreenNoteDisplayStateHandler
    : public DisplayForcedOffSetter::Observer,
      public chromeos::PowerManagerClient::Observer {
 public:
  explicit LockScreenNoteDisplayStateHandler(
      DisplayForcedOffSetter* display_forced_off_setter);
  ~LockScreenNoteDisplayStateHandler() override;

  // DisplayForcedOffSetter::Observer:
  void OnBacklightsForcedOffChanged() override;

  // chromeos::PowerManagerClient::Observer:
  void BrightnessChanged(int level, bool user_initiated) override;

  // If lock screen note action is available, it requests a new lock screen note
  // with launch reason set to stylus eject.
  void AttemptNoteLaunchForStylusEject();

  base::OneShotTimer* launch_timer_for_test() { return &launch_timer_; }

 private:
  // Runs lock screen note launcher, which start lock screen app launch.
  void RunLockScreenNoteLauncher();

  // Forces the display off.
  void ForceDisplayOff();

  // Whether the display is off, or is being forced off.
  bool IsDisplayOff();

  // Whether a lock screen note is currently being launched by |this|.
  bool NoteLaunchInProgress() const;

  // Called by |lock_screen_noteLauncher_| when lock screen note launch is done.
  void NoteLaunchDone(bool success);

  // Callback for power manager client request to get screen brightness.
  // Brightness is requested when the LockScreenNoteDisplayStateHandler is
  // created in order to determine screen state at that time.
  void GotInitialScreenBrightness(base::Optional<double> brightness);

  // Object user to force the display off.
  DisplayForcedOffSetter* const display_forced_off_setter_;

  // Whether the screen is turned on (determined by tracking screen brightness
  // reported by power manager).
  base::Optional<bool> screen_on_;

  // Whether lock screen note launch is delayed until the screen is reported to
  // be off - this is used if lock screen note launch is requested when display
  // has been forced off, but the power manager still reports screen to be on.
  bool note_launch_delayed_until_screen_off_ = false;

  std::unique_ptr<LockScreenNoteLauncher> lock_screen_note_launcher_;

  // Scoped display forced off request - this is returned by
  // |display_forced_off_setter_->ForceDisplayOff()|, and will keep the display
  // in forced-off state until it's reset.
  std::unique_ptr<ScopedDisplayForcedOff> display_forced_off_;

  // Timer used to set up timeout for lock screen note launch.
  base::OneShotTimer launch_timer_;

  ScopedObserver<DisplayForcedOffSetter, DisplayForcedOffSetter::Observer>
      display_forced_off_observer_;

  ScopedObserver<chromeos::PowerManagerClient,
                 chromeos::PowerManagerClient::Observer>
      power_manager_observer_;

  base::WeakPtrFactory<LockScreenNoteDisplayStateHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenNoteDisplayStateHandler);
};

}  // namespace ash

#endif  // ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_NOTE_DISPLAY_STATE_HANDLER_H_
