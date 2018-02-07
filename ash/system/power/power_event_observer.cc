// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/power/power_event_observer.h"

#include <map>
#include <utility>

#include "ash/public/cpp/config.h"
#include "ash/root_window_controller.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/shell_port.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/wallpaper/wallpaper_widget_controller.h"
#include "ash/wm/lock_state_controller.h"
#include "ash/wm/lock_state_observer.h"
#include "base/location.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/user_activity/user_activity_detector.h"
#include "ui/compositor/compositor.h"
#include "ui/display/manager/chromeos/display_configurator.h"

namespace ash {

namespace {

void OnSuspendDisplaysCompleted(const base::Closure& suspend_callback,
                                bool status) {
  suspend_callback.Run();
}

// Returns whether the screen should be locked when device is suspended.
bool ShouldLockOnSuspend() {
  SessionController* controller = ash::Shell::Get()->session_controller();

  return controller->ShouldLockScreenAutomatically() &&
         controller->CanLockScreen();
}

}  // namespace

PowerEventObserver::PowerEventObserver()
    : lock_state_(Shell::Get()->session_controller()->IsScreenLocked()
                      ? LockState::kLocked
                      : LockState::kNotLocked),
      session_observer_(this),
      compositor_observer_(this) {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(
      this);
}

PowerEventObserver::~PowerEventObserver() {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(
      this);
}

void PowerEventObserver::OnLockAnimationsComplete() {
  VLOG(1) << "Screen locker animations have completed.";
  if (lock_state_ != LockState::kLocking)
    return;

  lock_state_ = LockState::kLockedCompositingPending;

  StartTrackingCompositingState();
}

void PowerEventObserver::SuspendImminent(
    power_manager::SuspendImminent::Reason reason) {
  suspend_started_ = true;

  // Stop compositing immediately if
  // * the screen lock flow has already completed
  //  * screen is not locked, and should remain unlocked during suspend
  if (lock_state_ == LockState::kLocked ||
      (lock_state_ == LockState::kNotLocked && !ShouldLockOnSuspend())) {
    StopRootWindowCompositors();
  } else {
    // If screen is getting locked during suspend, delay suspend until screen
    // lock finishes, and post-lock frames get picked up by display compositors.
    compositors_stopped_callback_ =
        chromeos::DBusThreadManager::Get()
            ->GetPowerManagerClient()
            ->GetSuspendReadinessCallback(FROM_HERE);

    if (lock_state_ == LockState::kNotLocked) {
      VLOG(1) << "Requesting screen lock from PowerEventObserver";
      lock_state_ = LockState::kLocking;
      Shell::Get()->lock_state_controller()->LockWithoutAnimation();
    }
  }

  ui::UserActivityDetector::Get()->OnDisplayPowerChanging();

  // TODO(derat): After mus exposes a method for suspending displays, call it
  // here: http://crbug.com/692193
  if (Shell::GetAshConfig() != Config::MASH) {
    Shell::Get()->display_configurator()->SuspendDisplays(
        base::Bind(&OnSuspendDisplaysCompleted,
                   chromeos::DBusThreadManager::Get()
                       ->GetPowerManagerClient()
                       ->GetSuspendReadinessCallback(FROM_HERE)));
  }
}

void PowerEventObserver::SuspendDone(const base::TimeDelta& sleep_duration) {
  suspend_started_ = false;

  // TODO(derat): After mus exposes a method for resuming displays, call it
  // here: http://crbug.com/692193
  if (Shell::GetAshConfig() != Config::MASH)
    Shell::Get()->display_configurator()->ResumeDisplays();
  Shell::Get()->system_tray_notifier()->NotifyRefreshClock();

  // If the suspend request was being blocked while waiting for the lock
  // animation to complete, clear the blocker since the suspend has already
  // completed.  This prevents rendering requests from being blocked after a
  // resume if the lock screen took too long to show.
  compositors_stopped_callback_.Reset();

  StartRootWindowCompositors();
}

void PowerEventObserver::OnLockStateChanged(bool locked) {
  if (locked) {
    lock_state_ = LockState::kLocking;

    // The screen is now locked but the pending suspend, if any, will be blocked
    // until all the animations have completed.
    if (!compositors_stopped_callback_.is_null()) {
      VLOG(1) << "Screen locked due to suspend";
    } else {
      VLOG(1) << "Screen locked without suspend";
    }
  } else {
    lock_state_ = LockState::kNotLocked;
    pending_compositing_.clear();
    compositor_observer_.RemoveAll();

    if (suspend_started_) {
      LOG(WARNING) << "Screen locked during suspend";
      // If screen gets unlocked during suspend, which could teoretically happen
      // if the user initiated unlock just as device started unlocking (though,
      // it seems unlikely this would be encountered in practice), relock the
      // device if requred. Otherwise, if suspend is blocked due to screen
      // locking, unblock it.
      if (ShouldLockOnSuspend()) {
        lock_state_ = LockState::kLocking;
        Shell::Get()->lock_state_controller()->LockWithoutAnimation();
      } else if (!compositors_stopped_callback_.is_null()) {
        StopRootWindowCompositors();
      }
    }
  }
}

void PowerEventObserver::OnCompositingDidCommit(ui::Compositor* compositor) {
  if (!pending_compositing_.count(compositor) ||
      pending_compositing_[compositor] != CompositingState::kWaitingForCommit) {
    return;
  }
  pending_compositing_[compositor] = CompositingState::kWaitingForStarted;
}

void PowerEventObserver::OnCompositingStarted(ui::Compositor* compositor,
                                              base::TimeTicks start_time) {
  if (!pending_compositing_.count(compositor) ||
      pending_compositing_[compositor] !=
          CompositingState::kWaitingForStarted) {
    return;
  }
  pending_compositing_[compositor] = CompositingState::kWaitingForEnded;
}

void PowerEventObserver::OnCompositingEnded(ui::Compositor* compositor) {
  if (!pending_compositing_.count(compositor) ||
      pending_compositing_[compositor] != CompositingState::kWaitingForEnded) {
    return;
  }

  compositor_observer_.Remove(compositor);
  if (!compositors_stopped_callback_.is_null()) {
    pending_compositing_.erase(compositor);
    compositor->SetVisible(false);
  } else {
    pending_compositing_[compositor] = CompositingState::kEnded;
  }

  UpdateStateIfAllCompositingCyclesEnded();
}

void PowerEventObserver::OnCompositingLockStateChanged(
    ui::Compositor* compositor) {}

void PowerEventObserver::OnCompositingChildResizing(
    ui::Compositor* compositor) {}

void PowerEventObserver::OnCompositingShuttingDown(ui::Compositor* compositor) {
  compositor_observer_.Remove(compositor);
  pending_compositing_.erase(compositor);

  UpdateStateIfAllCompositingCyclesEnded();
}

void PowerEventObserver::StartRootWindowCompositors() {
  for (aura::Window* window : Shell::GetAllRootWindows()) {
    ui::Compositor* compositor = window->GetHost()->compositor();
    if (!compositor->IsVisible())
      compositor->SetVisible(true);
  }
}

void PowerEventObserver::StopRootWindowCompositors() {
  pending_compositing_.clear();
  compositor_observer_.RemoveAll();

  for (aura::Window* window : Shell::GetAllRootWindows()) {
    ui::Compositor* compositor = window->GetHost()->compositor();
    compositor->SetVisible(false);
  }

  if (!compositors_stopped_callback_.is_null())
    std::move(compositors_stopped_callback_).Run();
}

void PowerEventObserver::StartTrackingCompositingState() {
  for (aura::Window* window : Shell::GetAllRootWindows()) {
    ui::Compositor* compositor = window->GetHost()->compositor();
    if (!compositor->IsVisible())
      continue;

    if (pending_compositing_.count(compositor))
      continue;

    pending_compositing_[compositor] = CompositingState::kWaitingForCommit;
    compositor_observer_.Add(compositor);
    compositor->ScheduleDraw();
  }

  UpdateStateIfAllCompositingCyclesEnded();
}

void PowerEventObserver::UpdateStateIfAllCompositingCyclesEnded() {
  for (const auto& it : pending_compositing_) {
    if (it.second != CompositingState::kEnded)
      return;
  }

  lock_state_ = LockState::kLocked;

  if (!compositors_stopped_callback_.is_null())
    StopRootWindowCompositors();
}

}  // namespace ash
