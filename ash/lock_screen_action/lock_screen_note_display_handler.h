// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_NOTE_DISPLAY_HANDLER_H_
#define ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_NOTE_DISPLAY_HANDLER_H_

#include <memory>

#include "ash/system/power/display_power_controller_observer.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "base/timer/timer.h"
#include "ui/events/devices/input_device_event_observer.h"

namespace ui {
class InputDeviceManager;
}

namespace ash {

class DisplayPowerController;
class LockScreenNoteLauncher;
class ScopedDisplayForcedOff;

// Controls lock screen note action state interaction with display state.
// For example, it will close any active lock screen notes if the display is
// forced off, and it will launch a lock screen note if stylus is ejected
// while the screen is on, or forced off. If the note is launched as the
// display is stopping being forced off, |lock_screen_note_controller_| will
// be used to delay changing display state.
class LockScreenNoteDisplayHandler : public ui::InputDeviceEventObserver,
                                     public DisplayPowerControllerObserver {
 public:
  explicit LockScreenNoteDisplayHandler(
      DisplayPowerController* display_controller);
  ~LockScreenNoteDisplayHandler() override;

  // DisplayPowerController::Observer:
  void OnScreenStateChanged() override;
  void OnBacklightsForcedOffChanged() override;
  void OnWillHandleStylusRemoved() override;

  // ui::InputDeviceEventObserver:
  void OnStylusStateChanged(ui::StylusState state) override;

  base::OneShotTimer* launch_timer_for_test() { return &launch_timer_; }

 private:
  // If lock screen note action is available, it requests a new lock screen note
  // with launch reason set to stylus eject.
  void AttemptNoteLaunchForStylusEject();

  void RunLockScreenNoteLauncher();
  void ForceDisplayOff();
  bool IsDisplayOff();

  void NoteLaunchDone(bool success);

  DisplayPowerController* const display_controller_;

  bool note_launch_delayed_until_screen_off_ = false;

  std::unique_ptr<LockScreenNoteLauncher> lock_screen_note_launcher_;
  std::unique_ptr<ScopedDisplayForcedOff> display_forced_off_;

  // Timer used to set up timeout for lock screen note launch.
  base::OneShotTimer launch_timer_;

  ScopedObserver<ui::InputDeviceManager, ui::InputDeviceEventObserver>
      stylus_state_observer_;
  ScopedObserver<DisplayPowerController, DisplayPowerControllerObserver>
      display_observer_;

  base::WeakPtrFactory<LockScreenNoteDisplayHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(LockScreenNoteDisplayHandler);
};

}  // namespace ash

#endif  // ASH_LOCK_SCREEN_ACTION_LOCK_SCREEN_NOTE_DISPLAY_HANDLER_H_
