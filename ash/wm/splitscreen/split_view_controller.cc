// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/splitscreen/split_view_controller.h"

#include "ash/shell.h"
#include "ash/wm/overview/window_selector_item.h"
#include "ash/wm/widget_finder.h"
#include "ash/wm/window_positioning_utils.h"
#include "ash/wm/window_state.h"
#include "ash/wm/wm_event.h"
#include "ash/wm_window.h"
#include "base/logging.h"
#include "ui/aura/window.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace ash {

namespace {

// The time in milliseconds which should be used to restore a snapped window to
// its original position.
const int kWindowRestoreDurationMS = 250;

}  // namespace

SplitViewController::SplitViewController() {}

SplitViewController::~SplitViewController() {
  EndSplitView(false /* animate */);
}

bool SplitViewController::CanSnap(
    OverviewWindowDragController::SnapType snap_type) const {
  if (snap_type == OverviewWindowDragController::SNAP_LEFT)
    return !left_window_;
  else if (snap_type == OverviewWindowDragController::SNAP_RIGHT)
    return !right_window_;
  else
    return false;
}

bool SplitViewController::IsSplitViewModeActive() const {
  return state_ != INACTIVE;
}

void SplitViewController::SetLeftWindow(aura::Window* left_window) {
  if (!CanSnap(OverviewWindowDragController::SNAP_LEFT))
    return;

  left_window_ = left_window;
  left_window_original_bounds_ = left_window_->bounds();
  left_window_->AddObserver(this);
  wm::GetWindowState(left_window_)->AddObserver(this);
  const wm::WMEvent event(wm::WM_EVENT_SNAP_LEFT);
  wm::GetWindowState(left_window_)->OnWMEvent(&event);
  state_ = (state_ == INACTIVE) ? LEFT_ACTIVE : BOTH_ACTIVE;
  NotifySplitViewStateChanged(state_, left_window_);
}

void SplitViewController::SetRightWindow(aura::Window* right_window) {
  if (!CanSnap(OverviewWindowDragController::SNAP_RIGHT))
    return;

  right_window_ = right_window;
  right_window_original_bounds_ = right_window_->bounds();
  right_window_->AddObserver(this);
  wm::GetWindowState(right_window_)->AddObserver(this);
  const wm::WMEvent event(wm::WM_EVENT_SNAP_RIGHT);
  wm::GetWindowState(right_window_)->OnWMEvent(&event);
  state_ = (state_ == INACTIVE) ? RIGHT_ACTIVE : BOTH_ACTIVE;
  NotifySplitViewStateChanged(state_, right_window_);
}

void SplitViewController::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void SplitViewController::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void SplitViewController::OnWindowDestroying(aura::Window* window) {
  DCHECK(state_ != INACTIVE);

  // Close one of the snapped windows will end the split view mode.
  EndSplitView(true /* animate */);
}

void SplitViewController::OnPostWindowStateTypeChange(
    ash::wm::WindowState* window_state,
    ash::wm::WindowStateType old_type) {
  // If one of the snapped windows gets minimized / maximized / full-screened,
  // exit the split view mode.
  if (window_state->IsFullscreen() || window_state->IsMinimized() ||
      window_state->IsMaximized()) {
    EndSplitView(true /* animate */);
  }
}

void SplitViewController::EndSplitView(bool animate) {
  if (!left_window_ && !right_window_)
    return;

  state_ = INACTIVE;
  StopObservingAndRestore(left_window_, left_window_original_bounds_, animate);
  StopObservingAndRestore(right_window_, right_window_original_bounds_,
                          animate);
  left_window_ = nullptr;
  right_window_ = nullptr;
  NotifySplitViewStateChanged(state_, nullptr);
}

void SplitViewController::StopObservingAndRestore(
    aura::Window* window,
    const gfx::Rect original_bounds,
    bool animate) {
  if (!window)
    return;
  window->RemoveObserver(this);
  wm::GetWindowState(window)->RemoveObserver(this);

  if (animate) {
    WmWindow::Get(window)->SetBoundsWithTransitionDelay(
        original_bounds,
        base::TimeDelta::FromMilliseconds(kWindowRestoreDurationMS));
  } else {
    WmWindow::Get(window)->SetBounds(original_bounds);
  }
}

void SplitViewController::NotifySplitViewStateChanged(State state,
                                                      aura::Window* window) {
  for (Observer& observer : observers_)
    observer.OnSplitViewStateChanged(state, window);

  if (state == LEFT_ACTIVE || state == RIGHT_ACTIVE)
    Shell::Get()->NotifySplitViewModeStarted();
  else if (state == INACTIVE)
    Shell::Get()->NotifySplitViewModeEnded();
}

}  // namespace ash
