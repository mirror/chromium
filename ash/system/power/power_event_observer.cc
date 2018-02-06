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

}  // namespace

PowerEventObserver::PowerEventObserver()
    : lock_state_(Shell::Get()->session_controller()->IsScreenLocked()
                      ? LockState::kLocked
                      : LockState::kNotLocked),
      compositor_observer_(this),
      session_observer_(this) {
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

  StartObservingRootWindowCompositors();
}

void PowerEventObserver::SuspendImminent(
    power_manager::SuspendImminent::Reason reason) {
  SessionController* controller = Shell::Get()->session_controller();

  const bool should_lock = controller->ShouldLockScreenAutomatically() &&
                           controller->CanLockScreen();
  if ((lock_state_ == LockState::kNotLocked && !should_lock) ||
      lock_state_ == LockState::kLocked) {
    StopRootWindowCompositors();
  } else {
    compositors_stopped_callback_ =
        chromeos::DBusThreadManager::Get()
            ->GetPowerManagerClient()
            ->GetSuspendReadinessCallback(FROM_HERE);

    if (lock_state_ == LockState::kNotLocked && should_lock) {
      VLOG(1) << "Requesting screen lock from PowerEventObserver";
      lock_state_ = LockState::kLocking;
      Shell::Get()->lock_state_controller()->LockWithoutAnimation();
    } else if (lock_state_ != LockState::kLocking) {
      StartObservingRootWindowCompositors();
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

    if (!compositors_stopped_callback_.is_null())
      StopRootWindowCompositors();
  }
}

void PowerEventObserver::OnCompositingDidCommit(ui::Compositor* compositor) {
  if (!observed_compositors_.count(compositor) ||
      observed_compositors_[compositor] !=
          CompositorObserverState::kWaitingForCompositingCommit) {
    return;
  }
  observed_compositors_[compositor] =
      CompositorObserverState::kWaitingForCompositingStarted;
}

void PowerEventObserver::OnCompositingStarted(ui::Compositor* compositor,
                                              base::TimeTicks start_time) {
  if (!observed_compositors_.count(compositor) ||
      observed_compositors_[compositor] !=
          CompositorObserverState::kWaitingForCompositingStarted) {
    return;
  }
  observed_compositors_[compositor] =
      CompositorObserverState::kWaitingForCompositingEnded;
}

void PowerEventObserver::OnCompositingEnded(ui::Compositor* compositor) {
  if (!observed_compositors_.count(compositor) ||
      observed_compositors_[compositor] !=
          CompositorObserverState::kWaitingForCompositingEnded) {
    return;
  }

  compositor_observer_.Remove(compositor);
  if (!compositors_stopped_callback_.is_null()) {
    observed_compositors_.erase(compositor);
    compositor->SetVisible(false);
  } else {
    observed_compositors_[compositor] = CompositorObserverState::kReadyToStop;
  }

  UpdateStateIfAllCompositingEnded();
}

void PowerEventObserver::OnCompositingLockStateChanged(
    ui::Compositor* compositor) {}

void PowerEventObserver::OnCompositingChildResizing(
    ui::Compositor* compositor) {}

void PowerEventObserver::OnCompositingShuttingDown(ui::Compositor* compositor) {
  compositor_observer_.Remove(compositor);
  observed_compositors_.erase(compositor);

  UpdateStateIfAllCompositingEnded();
}

void PowerEventObserver::StartRootWindowCompositors() {
  for (aura::Window* window : Shell::GetAllRootWindows()) {
    ui::Compositor* compositor = window->GetHost()->compositor();
    if (!compositor->IsVisible())
      compositor->SetVisible(true);
  }
}

void PowerEventObserver::StopRootWindowCompositors() {
  observed_compositors_.clear();
  compositor_observer_.RemoveAll();

  for (aura::Window* window : Shell::GetAllRootWindows()) {
    ui::Compositor* compositor = window->GetHost()->compositor();
    compositor->SetVisible(false);
  }

  if (!compositors_stopped_callback_.is_null())
    std::move(compositors_stopped_callback_).Run();
}

void PowerEventObserver::StartObservingRootWindowCompositors() {
  for (aura::Window* window : Shell::GetAllRootWindows()) {
    ui::Compositor* compositor = window->GetHost()->compositor();
    if (!compositor->IsVisible())
      continue;

    if (observed_compositors_.count(compositor))
      continue;

    observed_compositors_[compositor] =
        CompositorObserverState::kWaitingForCompositingCommit;
    compositor_observer_.Add(compositor);
    compositor->ScheduleDraw();
  }

  UpdateStateIfAllCompositingEnded();
}

void PowerEventObserver::UpdateStateIfAllCompositingEnded() {
  // Bail out if any of the compositor has not gone through compositing.
  for (const auto& it : observed_compositors_) {
    if (it.second != CompositorObserverState::kReadyToStop)
      return;
  }

  lock_state_ = LockState::kLocked;

  if (!compositors_stopped_callback_.is_null())
    StopRootWindowCompositors();
}

}  // namespace ash
