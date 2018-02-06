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

// A class that observes power-management-related events.
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

  enum class CompositingState {
    kWaitingForCommit,
    kWaitingForStarted,
    kWaitingForEnded,
    kEnded
  };

  void StartRootWindowCompositors();
  void StopRootWindowCompositors();

  void StartObservingRootWindowCompositors();
  void UpdateStateIfAllCompositingEnded();

  LockState lock_state_;

  base::Closure compositors_stopped_callback_;

  std::map<ui::Compositor*, CompositingState> pending_compositing_;
  ScopedObserver<ui::Compositor, ui::CompositorObserver> compositor_observer_;

  ScopedSessionObserver session_observer_;

  DISALLOW_COPY_AND_ASSIGN(PowerEventObserver);
};

}  // namespace ash

#endif  // ASH_SYSTEM_POWER_POWER_EVENT_OBSERVER_H_
