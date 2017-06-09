// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/splitview/split_view_controller.h"

#include "ash/ash_switches.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/screen_util.h"
#include "ash/shell.h"
#include "ash/wm/maximize_mode/maximize_mode_controller.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "ash/wm/wm_event.h"
#include "base/command_line.h"
#include "ui/aura/window.h"
#include "ui/wm/core/coordinate_conversion.h"
#include "ui/wm/public/activation_client.h"

namespace ash {

namespace {

// Returns true if |window| can be activated and snapped.
bool CanSnap(aura::Window* window) {
  if (!wm::CanActivateWindow(window))
    return false;
  return wm::GetWindowState(window)->CanSnap();
}

}  // namespace

SplitViewController::SplitViewController() {
  Shell::Get()->AddShellObserver(this);
  Shell::Get()->activation_client()->AddObserver(this);
}

SplitViewController::~SplitViewController() {
  Shell::Get()->RemoveShellObserver(this);
  Shell::Get()->activation_client()->RemoveObserver(this);
  EndSplitView();
}

// static
bool SplitViewController::ShouldAllowSplitView() {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshEnableTabletSplitView)) {
    return false;
  }
  if (!Shell::Get()
           ->maximize_mode_controller()
           ->IsMaximizeModeWindowManagerEnabled()) {
    return false;
  }
  return true;
}

bool SplitViewController::IsSplitViewModeActive() const {
  return state_ != NOSNAP;
}

void SplitViewController::SetLeftWindow(aura::Window* left_window) {
  if (!left_window)
    return;

  if (state_ == NOSNAP)
    Shell::Get()->NotifySplitViewModeStarting();

  if (right_window_ == left_window) {
    StopObservingAndRestore(right_window_);
    right_window_ = nullptr;
  }
  if (left_window_ != left_window) {
    StopObservingAndRestore(left_window_);
    left_window_ = nullptr;
    left_window_ = left_window;
    left_window_->AddObserver(this);
    wm::GetWindowState(left_window_)->AddObserver(this);
  }

  State previous_state = state_;
  switch (state_) {
    case NOSNAP:
      default_snap_position_ = LEFT;
      state_ = LEFT_SNAPPED;
      break;
    case LEFT_SNAPPED:
      state_ = LEFT_SNAPPED;
      break;
    case RIGHT_SNAPPED:
    case BOTH_SNAPPED:
      state_ = BOTH_SNAPPED;
      break;
  }

  const wm::WMEvent event(wm::WM_EVENT_SNAP_LEFT);
  wm::GetWindowState(left_window_)->OnWMEvent(&event);
  wm::ActivateWindow(left_window_);

  NotifySplitViewStateChanged(previous_state, state_);
}

void SplitViewController::SetRightWindow(aura::Window* right_window) {
  if (!right_window)
    return;

  if (state_ == NOSNAP)
    Shell::Get()->NotifySplitViewModeStarting();

  if (left_window_ == right_window) {
    StopObservingAndRestore(left_window_);
    left_window_ = nullptr;
  }
  if (right_window_ != right_window) {
    StopObservingAndRestore(right_window_);
    right_window_ = nullptr;
    right_window_ = right_window;
    right_window_->AddObserver(this);
    wm::GetWindowState(right_window_)->AddObserver(this);
  }

  State previous_state = state_;
  switch (state_) {
    case NOSNAP:
      default_snap_position_ = RIGHT;
      state_ = RIGHT_SNAPPED;
      break;
    case RIGHT_SNAPPED:
      state_ = RIGHT_SNAPPED;
      break;
    case LEFT_SNAPPED:
    case BOTH_SNAPPED:
      state_ = BOTH_SNAPPED;
      break;
  }

  const wm::WMEvent event(wm::WM_EVENT_SNAP_RIGHT);
  wm::GetWindowState(right_window_)->OnWMEvent(&event);
  wm::ActivateWindow(right_window_);

  NotifySplitViewStateChanged(previous_state, state_);
}

aura::Window* SplitViewController::GetDefaultSnappedWindow() {
  if (default_snap_position_ == LEFT)
    return left_window_;
  if (default_snap_position_ == RIGHT)
    return right_window_;
  return nullptr;
}

gfx::Rect SplitViewController::GetLeftWindowBoundsInParent(
    aura::Window* window) {
  aura::Window* root_window = window->GetRootWindow();
  gfx::Rect bounds_in_parent = ScreenUtil::GetDisplayWorkAreaBoundsInParent(
      root_window->GetChildById(kShellWindowId_DefaultContainer));
  if (separator_position_ < 0)
    separator_position_ = bounds_in_parent.width() * 0.5f;
  gfx::Rect bounds(bounds_in_parent.x(), bounds_in_parent.y(),
                   separator_position_, bounds_in_parent.height());
  return bounds;
}

gfx::Rect SplitViewController::GetRightWindowBoundsInParent(
    aura::Window* window) {
  aura::Window* root_window = window->GetRootWindow();
  gfx::Rect bounds_in_parent = ScreenUtil::GetDisplayWorkAreaBoundsInParent(
      root_window->GetChildById(kShellWindowId_DefaultContainer));
  if (separator_position_ < 0)
    separator_position_ = bounds_in_parent.width() * 0.5f;
  gfx::Rect bounds(bounds_in_parent.x() + separator_position_,
                   bounds_in_parent.y(),
                   bounds_in_parent.width() - separator_position_,
                   bounds_in_parent.height());
  return bounds;
}

gfx::Rect SplitViewController::GetLeftWindowBoundsInScreen(
    aura::Window* window) {
  gfx::Rect bounds = GetLeftWindowBoundsInParent(window);
  ::wm::ConvertRectToScreen(window->GetRootWindow(), &bounds);
  return bounds;
}

gfx::Rect SplitViewController::GetRightWindowBoundsInScreen(
    aura::Window* window) {
  gfx::Rect bounds = GetRightWindowBoundsInParent(window);
  ::wm::ConvertRectToScreen(window->GetRootWindow(), &bounds);
  return bounds;
}

void SplitViewController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void SplitViewController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void SplitViewController::OnWindowDestroying(aura::Window* window) {
  // If one of the snapped window gets closed, end the split view mode. The
  // behavior might change in the future.
  DCHECK(window == left_window_ || window == right_window_);
  EndSplitView();
}

void SplitViewController::OnPostWindowStateTypeChange(
    ash::wm::WindowState* window_state,
    ash::wm::WindowStateType old_type) {
  if (window_state->IsFullscreen() || window_state->IsMinimized() ||
      window_state->IsMaximized()) {
    // TODO(xdai): Decide what to do if one of the snapped windows gets
    // minimized /maximized / full-screened. For now we end the split view mode
    // for simplicity.
    EndSplitView();
  }
}

void SplitViewController::OnWindowActivated(ActivationReason reason,
                                            aura::Window* gained_active,
                                            aura::Window* lost_active) {
  if (!IsSplitViewModeActive())
    return;

  // Only snap window that can be snapped but hasn't been snapped.
  if (!gained_active || gained_active == left_window_ ||
      gained_active == right_window_ || !CanSnap(gained_active)) {
    return;
  }

  // Only window in MRU list can be snapped.
  aura::Window::Windows windows =
      Shell::Get()->mru_window_tracker()->BuildMruWindowList();
  if (std::find(windows.begin(), windows.end(), gained_active) == windows.end())
    return;

  // Snap the window on the non-default side of the screen if split view mode
  // is active.
  if (default_snap_position_ == LEFT)
    SetRightWindow(gained_active);
  else
    SetLeftWindow(gained_active);
}

void SplitViewController::OnOverviewModeStarting() {
  // If split view mode is active, reset |state_| to make it be able to select
  // another window from overview window grid.
  if (IsSplitViewModeActive()) {
    DCHECK(state_ == BOTH_SNAPPED);
    if (default_snap_position_ == LEFT) {
      StopObservingAndRestore(right_window_);
      state_ = LEFT_SNAPPED;
    } else {
      StopObservingAndRestore(left_window_);
      state_ = RIGHT_SNAPPED;
    }
    NotifySplitViewStateChanged(BOTH_SNAPPED, state_);
  }
}

void SplitViewController::OnOverviewModeEnded() {
  // If split view mode is active but only has one snapped window, use the MRU
  // window list to auto select another window to snap.
  if (IsSplitViewModeActive() && state_ != BOTH_SNAPPED) {
    aura::Window::Windows windows =
        Shell::Get()->mru_window_tracker()->BuildMruWindowList();
    auto end = std::remove_if(windows.begin(), windows.end(),
                              std::not1(std::ptr_fun(&CanSnap)));
    windows.resize(end - windows.begin());
    for (auto* window : windows) {
      if (window != GetDefaultSnappedWindow()) {
        if (default_snap_position_ == LEFT)
          SetRightWindow(window);
        else
          SetLeftWindow(window);
        break;
      }
    }
  }
}

void SplitViewController::EndSplitView() {
  StopObservingAndRestore(left_window_);
  StopObservingAndRestore(right_window_);
  left_window_ = nullptr;
  right_window_ = nullptr;
  default_snap_position_ = LEFT;
  separator_position_ = -1;

  State previous_state = state_;
  state_ = NOSNAP;
  NotifySplitViewStateChanged(previous_state, state_);

  Shell::Get()->NotifySplitViewModeEnded();
}

void SplitViewController::StopObservingAndRestore(aura::Window* window) {
  if (window) {
    window->RemoveObserver(this);
    if (wm::GetWindowState(window))
      wm::GetWindowState(window)->RemoveObserver(this);
  }
}

void SplitViewController::NotifySplitViewStateChanged(State previous_state,
                                                      State state) {
  for (Observer& observer : observers_)
    observer.OnSplitViewStateChanged(previous_state, state);
}

}  // namespace ash
