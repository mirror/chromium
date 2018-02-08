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
#include "base/bind.h"
#include "base/location.h"
#include "base/scoped_observer.h"
#include "base/task_scheduler/post_task.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/user_activity/user_activity_detector.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/compositor_observer.h"
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

// One-shot class that runs a callback after all compositors start and
// complete a compositing cycle.
class CompositorWatcher : public ui::CompositorObserver {
 public:
  // |callback| - called when all visible  root window compositors complete
  //     a compositing cycle. It will not be called after CompositorWatcher
  //     instance is deleted, nor from the CompositorWatcher destructor.
  explicit CompositorWatcher(base::OnceClosure callback)
      : callback_(std::move(callback)),
        compositor_observer_(this),
        weak_ptr_factory_(this) {
    Start();
  }
  ~CompositorWatcher() override = default;

  // ui::CompositorObserver:
  void OnCompositingDidCommit(ui::Compositor* compositor) override {
    if (!pending_compositing_.count(compositor) ||
        pending_compositing_[compositor] !=
            CompositingState::kWaitingForCommit) {
      return;
    }
    pending_compositing_[compositor] = CompositingState::kWaitingForStarted;
  }
  void OnCompositingStarted(ui::Compositor* compositor,
                            base::TimeTicks start_time) override {
    if (!pending_compositing_.count(compositor) ||
        pending_compositing_[compositor] !=
            CompositingState::kWaitingForStarted) {
      return;
    }
    pending_compositing_[compositor] = CompositingState::kWaitingForEnded;
  }
  void OnCompositingEnded(ui::Compositor* compositor) override {
    if (!pending_compositing_.count(compositor) ||
        pending_compositing_[compositor] !=
            CompositingState::kWaitingForEnded) {
      return;
    }

    compositor_observer_.Remove(compositor);
    pending_compositing_.erase(compositor);

    RunCallbackIfAllCompositingEnded();
  }
  void OnCompositingLockStateChanged(ui::Compositor* compositor) override {}
  void OnCompositingChildResizing(ui::Compositor* compositor) override {}
  void OnCompositingShuttingDown(ui::Compositor* compositor) override {
    compositor_observer_.Remove(compositor);
    pending_compositing_.erase(compositor);

    RunCallbackIfAllCompositingEnded();
  }

 private:
  // CompositorWatcher observes compositors for compositing events, in order to
  // determine whether a compositing cycle has ended for all root window
  // compositors. This enum is used to track this cycle. Compositing goes
  // through the following states: DidCommit -> CompositingStarted ->
  // CompositingEnded.
  enum class CompositingState {
    kWaitingForCommit,
    kWaitingForStarted,
    kWaitingForEnded,
  };

  // Starts observing all visible root window compositors.
  void Start() {
    for (aura::Window* window : Shell::GetAllRootWindows()) {
      ui::Compositor* compositor = window->GetHost()->compositor();
      if (!compositor->IsVisible())
        continue;

      DCHECK(!pending_compositing_.count(compositor));
      pending_compositing_[compositor] = CompositingState::kWaitingForCommit;
      compositor_observer_.Add(compositor);

      // Schedule a draw to force at least one more compositing cycle.
      compositor->ScheduleDraw();
    }

    RunCallbackIfAllCompositingEnded();
  }

  // If all observed root window compositors have gone through compositing
  // cycle, runs |callback_|.
  void RunCallbackIfAllCompositingEnded() {
    if (!pending_compositing_.empty())
      return;

    if (callback_) {
      base::PostTask(FROM_HERE, base::BindOnce(&CompositorWatcher::RunCallback,
                                               weak_ptr_factory_.GetWeakPtr()));
    }
  }

  // Wrapper around callback bound to weak ptr to ensure the callback is not
  // run after CompositorWatcher instance is destroyed.
  void RunCallback() { std::move(callback_).Run(); }

  base::OnceClosure callback_;

  // Per-compositor post-lock compositing state tracked by |this|. The map will
  // not contain compositors that were not visible at the time when the screen
  // got locked - the main purpose of tracking compositing state is to determine
  // whether compositors can be safely stopped (i.e. their visibility set to
  // false), so there should be no need for tracking compositors that were
  // hidden to start with.
  std::map<ui::Compositor*, CompositingState> pending_compositing_;
  ScopedObserver<ui::Compositor, ui::CompositorObserver> compositor_observer_;

  base::WeakPtrFactory<CompositorWatcher> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CompositorWatcher);
};

}  // namespace

PowerEventObserver::PowerEventObserver()
    : lock_state_(Shell::Get()->session_controller()->IsScreenLocked()
                      ? LockState::kLocked
                      : LockState::kUnlocked),
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

  // compositor_watcher_ is owned by this, and the callback passed to it won't
  // be called  after |compositor_watcher_|'s destruction, so base::Unretained
  // is safe here.
  compositor_watcher_ = std::make_unique<CompositorWatcher>(base::BindOnce(
      &PowerEventObserver::OnCompositorsReady, base::Unretained(this)));
}

void PowerEventObserver::SuspendImminent(
    power_manager::SuspendImminent::Reason reason) {
  suspend_started_ = true;

  // Stop compositing immediately if
  // * the screen lock flow has already completed
  // * screen is not locked, and should remain unlocked during suspend
  if (lock_state_ == LockState::kLocked ||
      (lock_state_ == LockState::kUnlocked && !ShouldLockOnSuspend())) {
    StopRootWindowCompositors();
  } else {
    // If screen is getting locked during suspend, delay suspend until screen
    // lock finishes, and post-lock frames get picked up by display compositors.
    compositors_stopped_callback_ =
        chromeos::DBusThreadManager::Get()
            ->GetPowerManagerClient()
            ->GetSuspendReadinessCallback(FROM_HERE);

    if (lock_state_ == LockState::kUnlocked) {
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
    lock_state_ = LockState::kUnlocked;
    compositor_watcher_.reset();

    if (suspend_started_) {
      LOG(WARNING) << "Screen locked during suspend";
      // If screen gets unlocked during suspend, which could theoretically
      // happen if the user initiated unlock just as device started unlocking
      // (though, it seems unlikely this would be encountered in practice),
      // relock the device if required. Otherwise, if suspend is blocked due to
      // screen locking, unblock it.
      if (ShouldLockOnSuspend()) {
        lock_state_ = LockState::kLocking;
        Shell::Get()->lock_state_controller()->LockWithoutAnimation();
      } else if (!compositors_stopped_callback_.is_null()) {
        StopRootWindowCompositors();
      }
    }
  }
}

void PowerEventObserver::StartRootWindowCompositors() {
  for (aura::Window* window : Shell::GetAllRootWindows()) {
    ui::Compositor* compositor = window->GetHost()->compositor();
    if (!compositor->IsVisible())
      compositor->SetVisible(true);
  }
}

void PowerEventObserver::StopRootWindowCompositors() {
  DCHECK(!compositor_watcher_.get());

  for (aura::Window* window : Shell::GetAllRootWindows()) {
    ui::Compositor* compositor = window->GetHost()->compositor();
    compositor->SetVisible(false);
  }

  if (!compositors_stopped_callback_.is_null())
    std::move(compositors_stopped_callback_).Run();
}

void PowerEventObserver::OnCompositorsReady() {
  compositor_watcher_.reset();

  lock_state_ = LockState::kLocked;

  if (!compositors_stopped_callback_.is_null())
    StopRootWindowCompositors();
}

}  // namespace ash
