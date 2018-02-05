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

void PowerEventObserver::OnCompositingDidCommit(ui::Compositor* compositor) {}

void PowerEventObserver::OnCompositingStarted(ui::Compositor* compositor,
                                              base::TimeTicks start_time) {
  if (!compositors_with_pending_stop_.count(compositor))
    return;
  compositors_with_pending_stop_[compositor] =
      CompositorObserverState::kWaitingForCompositingEnded;
}

void PowerEventObserver::OnCompositingEnded(ui::Compositor* compositor) {
  if (!compositors_with_pending_stop_.count(compositor) ||
      compositors_with_pending_stop_[compositor] !=
          CompositorObserverState::kWaitingForCompositingEnded) {
    return;
  }

  compositor_observer_.Remove(compositor);
  compositors_with_pending_stop_.erase(compositor);

  compositor->SetVisible(false);

  RunScreenLockCallbackIfAllCompositingEnded();
}

void PowerEventObserver::OnCompositingLockStateChanged(
    ui::Compositor* compositor) {}

void PowerEventObserver::OnCompositingChildResizing(
    ui::Compositor* compositor) {}

void PowerEventObserver::OnCompositingShuttingDown(ui::Compositor* compositor) {
  compositor_observer_.Remove(compositor);
  compositors_with_pending_stop_.erase(compositor);

  RunScreenLockCallbackIfAllCompositingEnded();
}

void PowerEventObserver::StopRootWindowCompositors() {
  for (aura::Window* window : Shell::GetAllRootWindows()) {
    ui::Compositor* compositor = window->GetHost()->compositor();
    if (!compositor->IsVisible())
      continue;
    if (compositors_with_pending_stop_.count(compositor))
      continue;

    compositors_with_pending_stop_[compositor] =
        CompositorObserverState::kWaitingForCompositingStarted;
    compositor_observer_.Add(compositor);
    compositor->ScheduleDraw();
  }

  RunScreenLockCallbackIfAllCompositingEnded();
}

void PowerEventObserver::StartRootWindowCompositors() {
  compositors_with_pending_stop_.clear();
  compositor_observer_.RemoveAll();

  for (aura::Window* window : Shell::GetAllRootWindows()) {
    ui::Compositor* compositor = window->GetHost()->compositor();
    if (!compositor->IsVisible())
      compositor->SetVisible(true);
  }
}

void PowerEventObserver::RunScreenLockCallbackIfAllCompositingEnded() {
  if (!compositors_with_pending_stop_.empty())
    return;

  if (!screen_lock_callback_.is_null())
    std::move(screen_lock_callback_).Run();
}

PowerEventObserver::PowerEventObserver()
    : session_observer_(this),
      screen_locked_(Shell::Get()->session_controller()->IsScreenLocked()),
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
  waiting_for_lock_screen_animations_ = false;

  if (!screen_lock_callback_.is_null())
    StopRootWindowCompositors();
}

void PowerEventObserver::SuspendImminent(
    power_manager::SuspendImminent::Reason reason) {
  SessionController* controller = Shell::Get()->session_controller();

  screen_lock_callback_ = chromeos::DBusThreadManager::Get()
                              ->GetPowerManagerClient()
                              ->GetSuspendReadinessCallback(FROM_HERE);

  // This class is responsible for disabling all rendering requests at suspend
  // time and then enabling them at resume time.  When the
  // auto-screen-lock pref is not set this is easy to do since
  // StopRenderingRequests() is just called directly from this function.  If the
  // auto-screen-lock pref _is_ set, then the suspend needs to be delayed
  // until the lock screen is fully visible.  While it is sufficient from a
  // security perspective to block only until the lock screen is ready, which
  // guarantees that the contents of the user's screen are no longer visible,
  // this leads to poor UX on the first resume since neither the user pod nor
  // the header bar will be visible for a few hundred milliseconds until the GPU
  // process starts rendering again.  To deal with this, the suspend is delayed
  // until all the lock screen animations have completed and the suspend request
  // is unblocked from OnLockAnimationsComplete().
  if (!screen_locked_ && controller->ShouldLockScreenAutomatically() &&
      controller->CanLockScreen()) {
    VLOG(1) << "Requesting screen lock from PowerEventObserver";
    Shell::Get()->lock_state_controller()->LockWithoutAnimation();
  } else if (!waiting_for_lock_screen_animations_) {
    StopRootWindowCompositors();
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
  screen_lock_callback_.Reset();
  waiting_for_lock_screen_animations_ = false;

  StartRootWindowCompositors();
}

void PowerEventObserver::OnLockStateChanged(bool locked) {
  if (locked) {
    screen_locked_ = true;
    waiting_for_lock_screen_animations_ = true;

    // The screen is now locked but the pending suspend, if any, will be blocked
    // until all the animations have completed.
    if (!screen_lock_callback_.is_null()) {
      VLOG(1) << "Screen locked due to suspend";
    } else {
      VLOG(1) << "Screen locked without suspend";
    }
  } else {
    screen_locked_ = false;
  }
}

}  // namespace ash
