// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/splitview/split_view_controller.h"

#include "ash/ash_switches.h"
#include "ash/public/cpp/shell_window_ids.h"
#include "ash/screen_util.h"
#include "ash/shell.h"
#include "ash/wm/mru_window_tracker.h"
#include "ash/wm/splitview/split_view_divider.h"
#include "ash/wm/tablet_mode/tablet_mode_controller.h"
#include "ash/wm/window_state.h"
#include "ash/wm/window_util.h"
#include "ash/wm/wm_event.h"
#include "base/command_line.h"
#include "ui/aura/window.h"
#include "ui/compositor/layer.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/coordinate_conversion.h"
#include "ui/wm/public/activation_client.h"

namespace ash {

namespace {

// Five fixed position ratios of the divider.
constexpr float kFixedPositionRatios[] = {0.0f, 0.33f, 0.5f, 0.67f, 1.0f};

// Returns true if |window| can be activated and snapped.
bool CanSnap(aura::Window* window) {
  return wm::CanActivateWindow(window) ? wm::GetWindowState(window)->CanSnap()
                                       : false;
}

float FindClosestFixedPositionRatio(
    const gfx::Point& position,
    SplitViewController::SnapPosition default_snap_position,
    const gfx::Rect& work_area_bounds_in_screen) {
  float closest_ratio = 0.f;
  if (default_snap_position == SplitViewController::LEFT ||
      default_snap_position == SplitViewController::RIGHT) {
    float min_distance = work_area_bounds_in_screen.width();
    for (float ratio : kFixedPositionRatios) {
      float distance =
          std::abs(work_area_bounds_in_screen.x() +
                   ratio * work_area_bounds_in_screen.width() - position.x());
      if (distance < min_distance) {
        min_distance = distance;
        closest_ratio = ratio;
      }
    }
  } else if (default_snap_position == SplitViewController::TOP ||
             default_snap_position == SplitViewController::BOTTOM) {
    float min_distance = work_area_bounds_in_screen.height();
    for (float ratio : kFixedPositionRatios) {
      float distance =
          std::abs(work_area_bounds_in_screen.y() +
                   ratio * work_area_bounds_in_screen.height() - position.y());
      if (distance < min_distance) {
        min_distance = distance;
        closest_ratio = ratio;
      }
    }
  }
  return closest_ratio;
}

// Gets the corresponding WMEvent type according to |snap_position|.
wm::WMEventType GetWMEventTypeFromSnapPosition(
    SplitViewController::SnapPosition snap_position) {
  switch (snap_position) {
    case SplitViewController::LEFT:
      return wm::WM_EVENT_SNAP_LEFT;
    case SplitViewController::RIGHT:
      return wm::WM_EVENT_SNAP_RIGHT;
    case SplitViewController::TOP:
      return wm::WM_EVENT_SNAP_TOP;
    case SplitViewController::BOTTOM:
      return wm::WM_EVENT_SNAP_BOTTOM;
    case SplitViewController::NONE:
      NOTREACHED() << "Should not reach here.";
      return wm::WM_EVENT_NORMAL;
  }
}

}  // namespace

SplitViewController::SplitViewController() {}

SplitViewController::~SplitViewController() {
  EndSplitView();
}

// static
bool SplitViewController::ShouldAllowSplitView() {
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAshEnableTabletSplitView)) {
    return false;
  }
  if (!Shell::Get()
           ->tablet_mode_controller()
           ->IsTabletModeWindowManagerEnabled()) {
    return false;
  }
  return true;
}

bool SplitViewController::IsSplitViewModeActive() const {
  return state_ != NO_SNAP;
}

void SplitViewController::SnapWindow(aura::Window* window,
                                     SnapPosition snap_position) {
  DCHECK(window && CanSnap(window));
  DCHECK_NE(snap_position, NONE);

  if (state_ == NO_SNAP) {
    // Add observers when the split view mode starts.
    Shell::Get()->AddShellObserver(this);
    Shell::Get()->activation_client()->AddObserver(this);
    display::Screen::GetScreen()->AddObserver(this);
    Shell::Get()->NotifySplitViewModeStarting();

    const gfx::Rect work_area_bounds_in_screen =
        GetDisplayWorkAreaBoundsInScreen(window);
    divider_position_ =
        GetDefaultDividerPosition(work_area_bounds_in_screen, snap_position);
    default_snap_position_ = snap_position;
    split_view_divider_ =
        base::MakeUnique<SplitViewDivider>(this, window->GetRootWindow());
  }

  State previous_state = state_;
  if (snap_position == LEFT || snap_position == TOP) {
    if (left_window_ != window) {
      StopObserving(left_window_);
      left_window_ = window;
    }
    right_window_ = (window == right_window_) ? nullptr : right_window_;

    if (right_window_) {
      state_ = BOTH_SNAPPED;
    } else {
      state_ = (snap_position == LEFT) ? LEFT_SNAPPED : TOP_SNAPPED;
      default_snap_position_ = snap_position;
    }
  } else if (snap_position == RIGHT || snap_position == BOTTOM) {
    if (right_window_ != window) {
      StopObserving(right_window_);
      right_window_ = window;
    }
    left_window_ = (window == left_window_) ? nullptr : left_window_;

    if (left_window_) {
      state_ = BOTH_SNAPPED;
    } else {
      state_ = (snap_position == RIGHT) ? RIGHT_SNAPPED : BOTTOM_SNAPPED;
      default_snap_position_ = snap_position;
    }
  }

  StartObserving(window);
  const wm::WMEvent event(GetWMEventTypeFromSnapPosition(snap_position));
  wm::GetWindowState(window)->OnWMEvent(&event);
  wm::ActivateWindow(window);

  // Stack the other snapped window below the current active window so that
  // the snapped two windows are always the top two windows while resizing.
  aura::Window* stacking_target =
      (window == left_window_) ? right_window_ : left_window_;
  if (stacking_target)
    window->parent()->StackChildBelow(stacking_target, window);

  NotifySplitViewStateChanged(previous_state, state_);
}

aura::Window* SplitViewController::GetDefaultSnappedWindow() {
  if (default_snap_position_ == LEFT || default_snap_position_ == TOP)
    return left_window_;
  if (default_snap_position_ == RIGHT || default_snap_position_ == BOTTOM)
    return right_window_;
  return nullptr;
}

SplitViewController::SnapPosition SplitViewController::GetOppositeSnapPosition(
    SnapPosition snap_position) {
  switch (snap_position) {
    case LEFT:
      return RIGHT;
    case RIGHT:
      return LEFT;
    case TOP:
      return BOTTOM;
    case BOTTOM:
      return TOP;
    case NONE:
      return NONE;
  }
}

SplitViewController::SnapPosition
SplitViewController::GetAvailableSnapPositionFromSnapState(State snap_state) {
  switch (snap_state) {
    case LEFT_SNAPPED:
      return RIGHT;
    case RIGHT_SNAPPED:
      return LEFT;
    case TOP_SNAPPED:
      return BOTTOM;
    case BOTTOM_SNAPPED:
      return TOP;
    case BOTH_SNAPPED:
      return NONE;
    case NONE:
      NOTREACHED() << "Should not reach here! ";
      return NONE;
  }
}

gfx::Rect SplitViewController::GetSnappedWindowBoundsInParent(
    aura::Window* window,
    SnapPosition snap_position) {
  gfx::Rect bounds = GetSnappedWindowBoundsInScreen(window, snap_position);
  ::wm::ConvertRectFromScreen(window->GetRootWindow(), &bounds);
  return bounds;
}

gfx::Rect SplitViewController::GetSnappedWindowBoundsInScreen(
    aura::Window* window,
    SnapPosition snap_position) {
  const gfx::Rect display_bounds_in_screen =
      GetDisplayWorkAreaBoundsInScreen(window);
  const gfx::Size divider_size = SplitViewDivider::GetDividerSize(
      display_bounds_in_screen, snap_position, is_resizing_);
  divider_position_ =
      (divider_position_ < 0)
          ? GetDefaultDividerPosition(display_bounds_in_screen, snap_position)
          : divider_position_;
  switch (snap_position) {
    case LEFT:
      return gfx::Rect(display_bounds_in_screen.x(),
                       display_bounds_in_screen.y(),
                       divider_position_ - display_bounds_in_screen.x(),
                       display_bounds_in_screen.height());
    case RIGHT:
      return gfx::Rect(divider_position_ + divider_size.width(),
                       display_bounds_in_screen.y(),
                       display_bounds_in_screen.x() +
                           display_bounds_in_screen.width() -
                           divider_position_ - divider_size.width(),
                       display_bounds_in_screen.height());
    case TOP:
      return gfx::Rect(display_bounds_in_screen.x(),
                       display_bounds_in_screen.y(),
                       display_bounds_in_screen.width(),
                       divider_position_ - display_bounds_in_screen.y());
    case BOTTOM:
      return gfx::Rect(display_bounds_in_screen.x(),
                       divider_position_ + divider_size.height(),
                       display_bounds_in_screen.width(),
                       display_bounds_in_screen.y() +
                           display_bounds_in_screen.height() -
                           divider_position_ - divider_size.height());
    case NONE:
      return display_bounds_in_screen;
  }
}

gfx::Rect SplitViewController::GetDisplayWorkAreaBoundsInParent(
    aura::Window* window) const {
  aura::Window* root_window = window->GetRootWindow();
  return ScreenUtil::GetDisplayWorkAreaBoundsInParent(
      root_window->GetChildById(kShellWindowId_DefaultContainer));
}

gfx::Rect SplitViewController::GetDisplayWorkAreaBoundsInScreen(
    aura::Window* window) const {
  gfx::Rect bounds = GetDisplayWorkAreaBoundsInParent(window);
  ::wm::ConvertRectToScreen(window->GetRootWindow(), &bounds);
  return bounds;
}

void SplitViewController::StartResize(const gfx::Point& location_in_screen) {
  DCHECK(IsSplitViewModeActive());
  is_resizing_ = true;
  split_view_divider_->UpdateDividerBounds(is_resizing_);
  previous_event_location_ = location_in_screen;
}

void SplitViewController::Resize(const gfx::Point& location_in_screen) {
  DCHECK(IsSplitViewModeActive());
  if (!is_resizing_)
    return;

  // Calculate |divider_position_|.
  const int previous_divider_position = divider_position_;
  if (default_snap_position_ == LEFT || default_snap_position_ == RIGHT)
    divider_position_ += location_in_screen.x() - previous_event_location_.x();
  else
    divider_position_ += location_in_screen.y() - previous_event_location_.y();
  previous_event_location_ = location_in_screen;

  // Restack windows order if necessary.
  RestackWindows(previous_divider_position, divider_position_);

  // Update the black scrim layer's bounds and opacity.
  UpdateBlackScrim(location_in_screen);

  // Update the snapped window/windows and divider's position.
  UpdateSnappedWindowsAndDividerBounds();
}

void SplitViewController::EndResize(const gfx::Point& location_in_screen) {
  DCHECK(IsSplitViewModeActive());
  // TODO(xdai): Use fade out animation instead of just removing it.
  black_scrim_layer_.reset();
  is_resizing_ = false;

  // Find the closest fixed point for |location_in_screen| and update the
  // |divider_position_|.
  const gfx::Rect bounds =
      GetDisplayWorkAreaBoundsInScreen(GetDefaultSnappedWindow());
  float ratio = FindClosestFixedPositionRatio(location_in_screen,
                                              default_snap_position_, bounds);
  // Find the distance we're resizing along.
  int distance =
      (default_snap_position_ == LEFT || default_snap_position_ == RIGHT)
          ? bounds.width()
          : bounds.height();
  divider_position_ = ratio * distance;

  // If the divider is close to the edge of the screen, exit the split view.
  // Note that we should make sure the window which occupies the larger part of
  // the screen is the active window when leaving the split view mode.
  if (divider_position_ == 0) {
    if (right_window_)
      wm::ActivateWindow(right_window_);
    EndSplitView();
  } else if (divider_position_ == distance) {
    if (left_window_)
      wm::ActivateWindow(left_window_);
    EndSplitView();
  } else {
    // Updates the snapped window/windows and divider's position.
    UpdateSnappedWindowsAndDividerBounds();
  }
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
  DCHECK(IsSplitViewModeActive());
  DCHECK(window == left_window_ || window == right_window_);
  EndSplitView();
}

void SplitViewController::OnPostWindowStateTypeChange(
    ash::wm::WindowState* window_state,
    ash::wm::WindowStateType old_type) {
  DCHECK(IsSplitViewModeActive());

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
  DCHECK(IsSplitViewModeActive());

  // If |gained_active| was activated as a side effect of a window disposition
  // change, do nothing. For example, when a snapped window is closed, another
  // window will be activated before OnWindowDestroying() is called. We should
  // not try to snap another window in this case.
  if (reason == ActivationReason::WINDOW_DISPOSITION_CHANGED)
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
  SnapWindow(gained_active, GetOppositeSnapPosition(default_snap_position_));
}

void SplitViewController::OnOverviewModeStarting() {
  DCHECK(IsSplitViewModeActive());

  // If split view mode is active, reset |state_| to make it be able to select
  // another window from overview window grid.
  State previous_state = state_;
  if (default_snap_position_ == LEFT || default_snap_position_ == TOP) {
    StopObserving(right_window_);
    right_window_ = nullptr;
    state_ = (default_snap_position_ == LEFT) ? LEFT_SNAPPED : TOP_SNAPPED;
  } else if (default_snap_position_ == RIGHT ||
             default_snap_position_ == BOTTOM) {
    StopObserving(left_window_);
    left_window_ = nullptr;
    state_ = (default_snap_position_ == RIGHT) ? RIGHT_SNAPPED : BOTTOM_SNAPPED;
  }
  NotifySplitViewStateChanged(previous_state, state_);
}

void SplitViewController::OnOverviewModeEnded() {
  DCHECK(IsSplitViewModeActive());

  // If split view mode is active but only has one snapped window, use the MRU
  // window list to auto select another window to snap.
  if (state_ != BOTH_SNAPPED) {
    aura::Window::Windows windows =
        Shell::Get()->mru_window_tracker()->BuildMruWindowList();
    for (auto* window : windows) {
      if (CanSnap(window) && window != GetDefaultSnappedWindow()) {
        SnapWindow(window, GetOppositeSnapPosition(default_snap_position_));
        break;
      }
    }
  }
}

void SplitViewController::OnDisplayMetricsChanged(
    const display::Display& display,
    uint32_t metrics) {
  DCHECK(IsSplitViewModeActive());

  display::Display current_display =
      display::Screen::GetScreen()->GetDisplayNearestWindow(
          GetDefaultSnappedWindow());
  if (display.id() != current_display.id())
    return;

  // If screen has been rotated, update the windows snap state and position
  // accordingly.
  State previous_state = state_;
  if (metrics & DISPLAY_METRIC_ROTATION) {
    if (display.rotation() == display::Display::ROTATE_0 ||
        display.rotation() == display::Display::ROTATE_180) {
      default_snap_position_ =
          (default_snap_position_ == TOP)
              ? LEFT
              : ((default_snap_position_ == BOTTOM) ? RIGHT
                                                    : default_snap_position_);
      state_ = (state_ == TOP_SNAPPED)
                   ? LEFT_SNAPPED
                   : ((state_ == BOTTOM_SNAPPED) ? RIGHT_SNAPPED : state_);
    } else {
      DCHECK(display.rotation() == display::Display::ROTATE_90 ||
             display.rotation() == display::Display::ROTATE_270);
      // If screen has been rotated, update the windows snap state and position
      // accordingly.
      default_snap_position_ =
          (default_snap_position_ == LEFT)
              ? TOP
              : ((default_snap_position_ == RIGHT) ? BOTTOM
                                                   : default_snap_position_);
      state_ = (state_ == LEFT_SNAPPED)
                   ? TOP_SNAPPED
                   : ((state_ == RIGHT_SNAPPED) ? BOTTOM_SNAPPED : state_);
    }
  }

  UpdateSnappedWindowsAndDividerBounds();
  NotifySplitViewStateChanged(previous_state, state_);
}

void SplitViewController::EndSplitView() {
  if (!IsSplitViewModeActive())
    return;

  // Remove observers when the split view mode ends.
  Shell::Get()->RemoveShellObserver(this);
  Shell::Get()->activation_client()->RemoveObserver(this);
  display::Screen::GetScreen()->RemoveObserver(this);

  StopObserving(left_window_);
  StopObserving(right_window_);
  left_window_ = nullptr;
  right_window_ = nullptr;
  split_view_divider_.reset();
  black_scrim_layer_.reset();
  default_snap_position_ = NONE;
  divider_position_ = -1;

  State previous_state = state_;
  state_ = NO_SNAP;
  NotifySplitViewStateChanged(previous_state, state_);

  Shell::Get()->NotifySplitViewModeEnded();
}

void SplitViewController::StartObserving(aura::Window* window) {
  if (window && !window->HasObserver(this)) {
    window->AddObserver(this);
    wm::GetWindowState(window)->AddObserver(this);
    split_view_divider_->AddObservedWindow(window);
  }
}

void SplitViewController::StopObserving(aura::Window* window) {
  if (window && window->HasObserver(this)) {
    window->RemoveObserver(this);
    wm::GetWindowState(window)->RemoveObserver(this);
    split_view_divider_->RemoveObservedWindow(window);
  }
}

void SplitViewController::NotifySplitViewStateChanged(State previous_state,
                                                      State state) {
  if (previous_state == state)
    return;
  for (Observer& observer : observers_)
    observer.OnSplitViewStateChanged(previous_state, state);
}

int SplitViewController::GetDefaultDividerPosition(
    const gfx::Rect& work_area_bounds_in_screen,
    SnapPosition snap_position) const {
  const gfx::Size divider_size = SplitViewDivider::GetDividerSize(
      work_area_bounds_in_screen, snap_position, is_resizing_);
  if (snap_position == LEFT || snap_position == RIGHT) {
    return work_area_bounds_in_screen.x() +
           (work_area_bounds_in_screen.width() - divider_size.width()) * 0.5f;
  } else {
    DCHECK(snap_position == TOP || snap_position == BOTTOM);
    return work_area_bounds_in_screen.y() +
           (work_area_bounds_in_screen.height() - divider_size.height()) * 0.5f;
  }
}

void SplitViewController::UpdateBlackScrim(
    const gfx::Point& location_in_screen) {
  DCHECK(IsSplitViewModeActive());
  DCHECK_NE(default_snap_position_, NONE);

  if (!black_scrim_layer_) {
    // Create an invisible black scrim layer.
    black_scrim_layer_ = base::MakeUnique<ui::Layer>(ui::LAYER_SOLID_COLOR);
    black_scrim_layer_->SetColor(SK_ColorBLACK);
    GetDefaultSnappedWindow()->GetRootWindow()->layer()->Add(
        black_scrim_layer_.get());
    GetDefaultSnappedWindow()->GetRootWindow()->layer()->StackAtTop(
        black_scrim_layer_.get());
  }

  gfx::Rect black_scrim_bounds;
  const gfx::Rect work_area_bounds =
      GetDisplayWorkAreaBoundsInScreen(GetDefaultSnappedWindow());
  float opacity = 0.f;

  if (default_snap_position_ == LEFT || default_snap_position_ == RIGHT) {
    if (location_in_screen.x() <
        work_area_bounds.x() +
            work_area_bounds.width() * kFixedPositionRatios[1]) {
      black_scrim_bounds =
          gfx::Rect(work_area_bounds.x(), work_area_bounds.y(),
                    location_in_screen.x() - work_area_bounds.x(),
                    work_area_bounds.height());
    } else if (location_in_screen.x() >
               work_area_bounds.x() +
                   work_area_bounds.width() * kFixedPositionRatios[3]) {
      black_scrim_bounds =
          gfx::Rect(location_in_screen.x(), work_area_bounds.y(),
                    work_area_bounds.x() + work_area_bounds.width() -
                        location_in_screen.x(),
                    work_area_bounds.height());
    } else {
      black_scrim_layer_->SetOpacity(0.f);
      return;
    }

    // Dynamically increase the black scrim opacity as it gets closer to the
    // edge of the screen.
    int distance_x =
        std::min(std::abs(location_in_screen.x() - work_area_bounds.x()),
                 std::abs(work_area_bounds.x() + work_area_bounds.width() -
                          location_in_screen.x()));
    opacity = 1.f - float(distance_x) / float(work_area_bounds.width());
  } else {
    DCHECK(default_snap_position_ == TOP || default_snap_position_ == BOTTOM);

    if (location_in_screen.y() <
        work_area_bounds.y() +
            work_area_bounds.height() * kFixedPositionRatios[1]) {
      black_scrim_bounds = gfx::Rect(
          work_area_bounds.x(), work_area_bounds.y(), work_area_bounds.width(),
          location_in_screen.y() - work_area_bounds.y());
    } else if (location_in_screen.y() >
               work_area_bounds.y() +
                   work_area_bounds.height() * kFixedPositionRatios[3]) {
      black_scrim_bounds =
          gfx::Rect(work_area_bounds.x(), location_in_screen.y(),
                    work_area_bounds.width(),
                    work_area_bounds.y() + work_area_bounds.height() -
                        location_in_screen.y());
    } else {
      black_scrim_layer_->SetOpacity(0.f);
      return;
    }

    // Dynamically increase the black scrim opacity as it gets closer to the
    // edge of the screen.
    int distance_y =
        std::min(std::abs(location_in_screen.y() - work_area_bounds.y()),
                 std::abs(work_area_bounds.y() + work_area_bounds.height() -
                          location_in_screen.y()));
    opacity = 1.f - float(distance_y) / float(work_area_bounds.height());
  }

  // Updates the black scrim layer bounds.
  black_scrim_layer_->SetBounds(black_scrim_bounds);
  black_scrim_layer_->SetOpacity(opacity);
}

void SplitViewController::RestackWindows(const int previous_divider_position,
                                         const int current_divider_position) {
  if (!left_window_ || !right_window_)
    return;
  DCHECK(IsSplitViewModeActive());
  DCHECK_EQ(left_window_->parent(), right_window_->parent());

  const gfx::Rect work_area_bounds_in_screen =
      GetDisplayWorkAreaBoundsInScreen(GetDefaultSnappedWindow());
  const int mid_position = GetDefaultDividerPosition(work_area_bounds_in_screen,
                                                     default_snap_position_);
  if (previous_divider_position >= mid_position &&
      current_divider_position < mid_position) {
    left_window_->parent()->StackChildAbove(right_window_, left_window_);
  } else if (previous_divider_position <= mid_position &&
             current_divider_position > mid_position) {
    left_window_->parent()->StackChildAbove(left_window_, right_window_);
  }
}

void SplitViewController::UpdateSnappedWindowsAndDividerBounds() {
  DCHECK(IsSplitViewModeActive());

  // Update the snapped windows' bounds.
  if (default_snap_position_ == LEFT || default_snap_position_ == RIGHT) {
    if (left_window_ && wm::GetWindowState(left_window_)->IsSnapped()) {
      const wm::WMEvent left_window_event(wm::WM_EVENT_SNAP_LEFT);
      wm::GetWindowState(left_window_)->OnWMEvent(&left_window_event);
    }
    if (right_window_ && wm::GetWindowState(right_window_)->IsSnapped()) {
      const wm::WMEvent right_window_event(wm::WM_EVENT_SNAP_RIGHT);
      wm::GetWindowState(right_window_)->OnWMEvent(&right_window_event);
    }
  } else {
    DCHECK(default_snap_position_ == TOP || default_snap_position_ == BOTTOM);
    if (left_window_ && wm::GetWindowState(left_window_)->IsSnapped()) {
      const wm::WMEvent top_window_event(wm::WM_EVENT_SNAP_TOP);
      wm::GetWindowState(left_window_)->OnWMEvent(&top_window_event);
    }
    if (right_window_ && wm::GetWindowState(right_window_)->IsSnapped()) {
      const wm::WMEvent bottom_window_event(wm::WM_EVENT_SNAP_BOTTOM);
      wm::GetWindowState(right_window_)->OnWMEvent(&bottom_window_event);
    }
  }

  // Update divider's bounds.
  split_view_divider_->UpdateDividerBounds(is_resizing_);
}

}  // namespace ash
