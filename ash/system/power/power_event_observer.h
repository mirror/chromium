// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_POWER_EVENT_OBSERVER_H_
#define ASH_SYSTEM_POWER_POWER_EVENT_OBSERVER_H_

#include <map>
#include <memory>
#include <vector>

#include "ash/ash_export.h"
#include "ash/session/session_observer.h"
#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/scoped_observer.h"
#include "chromeos/dbus/power_manager_client.h"
#include "ui/compositor/compositor_observer.h"

namespace ui {
class Compositor;
}

namespace ash {

// A class that observes power-management-related events - in particular, it
// handles device suspend events.
// It updates the display power status depending on the suspend state.
// When device suspends, it suspends all the displays and stops compositing all
// root windows. On resume, displays are resumed, and compositing started again.
// During suspend, the class takes special care to ensure compositing is not
// stopped prematurely if the device screen is locked - before a compositor is
// stopped the class ensures that:
//  1. lock screen window gets shown
//  2. the compositor goes through at least one compositing cycle after screen
//     lock.
// This is to ensure that displays have frames set after screen lock when the
// device is resumed (when compositing is stopped, subsequent frames will be
// ignred - on resume the display will show the last sumbitted frame). For
// example, see https://crbug.com/807511
class ASH_EXPORT PowerEventObserver
    : public chromeos::PowerManagerClient::Observer,
      public SessionObserver,
      public ui::CompositorObserver {
 public:
  // This class registers/unregisters itself as an observer in ctor/dtor.
  PowerEventObserver();
  ~PowerEventObserver() override;

  // Called by the WebUIScreenLocker when all the lock screen animations have
  // completed.  This really should be implemented via an observer but since
  // ash/ isn't allowed to depend on chrome/ we need to have the
  // WebUIScreenLocker reach into ash::Shell to make this call.
  void OnLockAnimationsComplete();

  // chromeos::PowerManagerClient::Observer overrides:
  void SuspendImminent(power_manager::SuspendImminent::Reason reason) override;
  void SuspendDone(const base::TimeDelta& sleep_duration) override;

  // SessionObserver overrides:
  void OnLockStateChanged(bool locked) override;

  // ui::CompositorObserver:
  void OnCompositingDidCommit(ui::Compositor* compositor) override;
  void OnCompositingStarted(ui::Compositor* compositor,
                            base::TimeTicks start_time) override;
  void OnCompositingEnded(ui::Compositor* compositor) override;
  void OnCompositingLockStateChanged(ui::Compositor* compositor) override;
  void OnCompositingChildResizing(ui::Compositor* compositor) override;
  void OnCompositingShuttingDown(ui::Compositor* compositor) override;

 private:
  enum class LockState {
    // Screen lock has not been requested, nor detected.
    kNotLocked,
    // Screen lock has been requested, or detected, but screen lock has not
    // reported that it's shown.
    kLocking,
    // Screen has been locked, but all compositors might not have yet picked up
    // locked screen state - |this| is observing compositor for a pair of
    // CompositingStarted, CompositingEnded events.
    kLockedCompositingPending,
    // Screen is locked, and compositors have gone through at least compositing
    // cycle with the screen locked - it should be safe to stop compositing at
    // this time.
    kLocked
  };

  // After screen lock, PowerEventObserver will start observing compositors for
  // compositing events, in order to determine whether a compositing cycle has
  // ended for root windows' compositors (suspend requests will be delayed until
  // all root window compositors go through at least one compositing cycle).
  // This enum is used to track this cycle. Compositing goes through the
  // following states: DidCommit -> CompositingStarted -> CompositingEnded.
  enum class CompositingState {
    kWaitingForCommit,
    kWaitingForStarted,
    kWaitingForEnded,
    kEnded
  };

  // Sets all root windows compositors visibility to true.
  void StartRootWindowCompositors();

  // Sets all root windows compositors visibility to false.
  // This should only be called when it's save to stop compositing -
  // either if the screen is not expected to get locked, or all compositors
  // have gone through compositing cycle after the screen was locked.
  void StopRootWindowCompositors();

  void StartTrackingCompositingState();
  void UpdateStateIfAllCompositingCyclesEnded();

  // The device lock state.
  LockState lock_state_;
  ScopedSessionObserver session_observer_;

  // Callback set when device suspend is delayed to wait for display compositors
  // to pick up frames from after the screen has been locked. All compositors
  // should be stopped prior to calling this - call StopRootWindowCompositors()
  // instead of runnig this callback directly.
  // This will only be set while the device is suspending.
  base::Closure compositors_stopped_callback_;

  // Per-compositor post-lock compositing state tracked by |this|. The map will
  // not contain compositors that were not visible at the time when the screen
  // got locked - the main purpose of tracking compositing state is to determine
  // whether compositors can be safely stopped (i.e. their visibility set to
  // false), so there should be no need for tracking compositors that were
  // hidden to start with.
  std::map<ui::Compositor*, CompositingState> pending_compositing_;
  ScopedObserver<ui::Compositor, ui::CompositorObserver> compositor_observer_;

  DISALLOW_COPY_AND_ASSIGN(PowerEventObserver);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_POWER_EVENT_OBSERVER_H_
