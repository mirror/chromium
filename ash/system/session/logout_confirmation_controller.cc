// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/session/logout_confirmation_controller.h"

#include <utility>

#include "ash/login_status.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/system/session/logout_confirmation_dialog.h"
#include "base/location.h"
#include "base/time/default_tick_clock.h"
#include "base/time/tick_clock.h"
#include "ui/aura/window.h"
#include "ui/views/widget/widget.h"

namespace ash {
namespace {
const int kLogoutConfirmationDelayInSeconds = 20;

// Shell window containers monitored for when the last window closes.
const int kLastWindowClosedContainerIds[] = {
    kShellWindowId_DefaultContainer, kShellWindowId_AlwaysOnTopContainer};
}

LogoutConfirmationController::LogoutConfirmationController(
    const base::Closure& logout_closure)
    : clock_(new base::DefaultTickClock),
      logout_closure_(logout_closure),
      logout_timer_(false, false),
      scoped_session_observer_(this) {}

LogoutConfirmationController::~LogoutConfirmationController() {
  if (dialog_)
    dialog_->ControllerGone();
}

void LogoutConfirmationController::ObserveForLastWindowClosed(
    aura::Window* root) {
  for (int id : kLastWindowClosedContainerIds) {
    root->GetChildById(id)->AddObserver(this);
  }
}

void LogoutConfirmationController::ConfirmLogout(base::TimeTicks logout_time) {
  if (!logout_time_.is_null() && logout_time >= logout_time_) {
    // If a confirmation dialog is already being shown and its countdown expires
    // no later than the |logout_time| requested now, keep the current dialog
    // open.
    return;
  }
  logout_time_ = logout_time;

  if (!dialog_) {
    // Show confirmation dialog unless this is a unit test without a Shell.
    if (Shell::HasInstance())
      dialog_ = new LogoutConfirmationDialog(this, logout_time_);
  } else {
    dialog_->Update(logout_time_);
  }

  logout_timer_.Start(FROM_HERE, logout_time_ - clock_->NowTicks(),
                      logout_closure_);
}

void LogoutConfirmationController::SetClockForTesting(
    std::unique_ptr<base::TickClock> clock) {
  clock_ = std::move(clock);
}

void LogoutConfirmationController::OnLockStateChanged(bool locked) {
  if (!locked || logout_time_.is_null())
    return;

  // If the screen is locked while a confirmation dialog is being shown, close
  // the dialog.
  logout_time_ = base::TimeTicks();
  if (dialog_)
    dialog_->GetWidget()->Close();
  logout_timer_.Stop();
}

void LogoutConfirmationController::OnLogoutConfirmed() {
  logout_timer_.Stop();
  logout_closure_.Run();
}

void LogoutConfirmationController::OnDialogClosed() {
  logout_time_ = base::TimeTicks();
  dialog_ = NULL;
  logout_timer_.Stop();
}

void LogoutConfirmationController::OnWindowHierarchyChanging(
    const HierarchyChangeParams& params) {
  if (!params.new_parent && params.old_parent) {
    // A window is being removed (and not moved to another container).
    ShowDialogIfLastWindowClosing();
  }
}

void LogoutConfirmationController::OnWindowDestroying(aura::Window* window) {
  // Stop observing the container window when it closes.
  window->RemoveObserver(this);
}

void LogoutConfirmationController::ShowDialogIfLastWindowClosing() {
  // Only ask the user to confirm logout if a public session is in progress
  // (which implies the screen is not locked).
  if (Shell::Get()->session_controller()->login_status() !=
      LoginStatus::PUBLIC) {
    return;
  }

  int window_count = 0;
  for (aura::Window* root : Shell::GetAllRootWindows()) {
    for (int id : kLastWindowClosedContainerIds)
      window_count += root->GetChildById(id)->children().size();
  }

  // Prompt if the Last window is closing.
  if (window_count == 1) {
    ConfirmLogout(
        base::TimeTicks::Now() +
        base::TimeDelta::FromSeconds(kLogoutConfirmationDelayInSeconds));
  }
}

}  // namespace ash
