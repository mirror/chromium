// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/splitview/split_view_controller.h"

#include "ash/ash_switches.h"
#include "ash/display/screen_orientation_controller_chromeos.h"
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

float FindClosestFixedPositionRatio(const gfx::Point& location_in_screen,
                                    const gfx::Rect& work_area_bounds,
                                    bool is_landscape) {
  float closest_ratio = 0.f;
  float min_distance =
      is_landscape ? work_area_bounds.width() : work_area_bounds.height();
  for (float ratio : kFixedPositionRatios) {
    float distance = is_landscape ? std::abs(work_area_bounds.x() +
                                             ratio * work_area_bounds.width() -
                                             location_in_screen.x())
                                  : std::abs(work_area_bounds.y() +
                                             ratio * work_area_bounds.height() -
                                             location_in_screen.y());
    if (distance < min_distance) {
      min_distance = distance;
      closest_ratio = ratio;
    }
  }
  return closest_ratio;
}

gfx::Point GetBoundedPosition(const gfx::Point& location_in_screen,
                              const gfx::Rect& bounds_in_screen) {
  return gfx::Point(
      std::max(std::min(location_in_screen.x(), bounds_in_screen.right() - 1),
               bounds_in_screen.x()),
      std::max(std::min(location_in_screen.y(), bounds_in_screen.bottom() - 1),
               bounds_in_screen.y()));
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

// static
bool SplitViewController::CanSnap(aura::Window* window) {
  return wm::CanActivateWindow(window) ? wm::GetWindowState(window)->CanSnap()
                                       : false;
}

bool SplitViewController::IsSplitViewModeActive() const {
  return state_ != NO_SNAP;
}

bool SplitViewController::IsCurrentScreenOrientationLandscape() const {
  return screen_orientation_ ==
             blink::kWebScreenOrientationLockLandscapePrimary ||
         screen_orientation_ ==
             blink::kWebScreenOrientationLockLandscapeSecondary;
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

    screen_orientation_ =
        Shell::Get()->screen_orientation_controller()->GetCurrentOrientation();
    divider_position_ = GetDefaultDividerPosition(window);
    split_view_divider_ =
        base::MakeUnique<SplitViewDivider>(this, window->GetRootWindow());
  }

  State previous_state = state_;
  if (snap_position == LEFT) {
    if (left_window_ != window) {
      StopObserving(left_window_);
      left_window_ = window;
    }
    right_window_ = (window == right_window_) ? nullptr : right_window_;

    if (right_window_) {
      state_ = BOTH_SNAPPED;
    } else {
      state_ = LEFT_SNAPPED;
      default_snap_position_ = LEFT;
    }
  } else if (snap_position == RIGHT) {
    if (right_window_ != window) {
      StopObserving(right_window_);
      right_window_ = window;
    }
    left_window_ = (window == left_window_) ? nullptr : left_window_;

    if (left_window_) {
      state_ = BOTH_SNAPPED;
    } else {
      state_ = RIGHT_SNAPPED;
      default_snap_position_ = RIGHT;
    }
  }

  StartObserving(window);
  const wm::WMEvent event((snap_position == LEFT) ? wm::WM_EVENT_SNAP_LEFT
                                                  : wm::WM_EVENT_SNAP_RIGHT);
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

void SplitViewController::SwapWindows() {
  if (state_ != BOTH_SNAPPED)
    return;

  DCHECK(left_window_ && right_window_);

  aura::Window* new_left_window = right_window_;
  aura::Window* new_right_window = left_window_;

  SnapWindow(new_left_window, LEFT);
  SnapWindow(new_right_window, RIGHT);
}

aura::Window* SplitViewController::GetDefaultSnappedWindow() {
  if (default_snap_position_ == LEFT)
    return left_window_;
  if (default_snap_position_ == RIGHT)
    return right_window_;
  return nullptr;
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
  if (snap_position == NONE)
    return display_bounds_in_screen;

  // |screen_orientation_| and |divide_position_| might not be properly
  // initialized yet.
  screen_orientation_ =
      Shell::Get()->screen_orientation_controller()->GetCurrentOrientation();
  divider_position_ = (divider_position_ < 0)
                          ? GetDefaultDividerPosition(window)
                          : divider_position_;
  const gfx::Size divider_size = SplitViewDivider::GetDividerSize(
      display_bounds_in_screen, screen_orientation_, false /* is_dragging */);

  switch (screen_orientation_) {
    case blink::kWebScreenOrientationLockLandscapePrimary:
      if (snap_position == LEFT) {
        return gfx::Rect(display_bounds_in_screen.x(),
                         display_bounds_in_screen.y(), divider_position_,
                         display_bounds_in_screen.height());
      } else {
        DCHECK_EQ(snap_position, RIGHT);
        return gfx::Rect(display_bounds_in_screen.x() + divider_position_ +
                             divider_size.width(),
                         display_bounds_in_screen.y(),
                         display_bounds_in_screen.width() - divider_position_ -
                             divider_size.width(),
                         display_bounds_in_screen.height());
      }
    case blink::kWebScreenOrientationLockPortraitPrimary:
      if (snap_position == LEFT) {
        return gfx::Rect(display_bounds_in_screen.x(),
                         display_bounds_in_screen.y() +
                             display_bounds_in_screen.height() -
                             divider_position_,
                         display_bounds_in_screen.width(), divider_position_);
      } else {
        DCHECK_EQ(snap_position, RIGHT);
        return gfx::Rect(display_bounds_in_screen.x(),
                         display_bounds_in_screen.y(),
                         display_bounds_in_screen.width(),
                         display_bounds_in_screen.height() - divider_position_ -
                             divider_size.height());
      }
    case blink::kWebScreenOrientationLockLandscapeSecondary:
      if (snap_position == LEFT) {
        return gfx::Rect(display_bounds_in_screen.x() +
                             display_bounds_in_screen.width() -
                             divider_position_,
                         display_bounds_in_screen.y(), divider_position_,
                         display_bounds_in_screen.height());
      } else {
        DCHECK_EQ(snap_position, RIGHT);
        return gfx::Rect(display_bounds_in_screen.x(),
                         display_bounds_in_screen.y(),
                         display_bounds_in_screen.width() - divider_position_ -
                             divider_size.width(),
                         display_bounds_in_screen.height());
      }
    case blink::kWebScreenOrientationLockPortraitSecondary:
      if (snap_position == LEFT) {
        return gfx::Rect(display_bounds_in_screen.x(),
                         display_bounds_in_screen.y(),
                         display_bounds_in_screen.width(), divider_position_);
      } else {
        DCHECK_EQ(snap_position, RIGHT);
        return gfx::Rect(display_bounds_in_screen.x(),
                         display_bounds_in_screen.y() + divider_position_ +
                             divider_size.height(),
                         display_bounds_in_screen.width(),
                         display_bounds_in_screen.height() - divider_position_ -
                             divider_size.height());
      }
    default:
      NOTREACHED();
      return gfx::Rect();
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

  const gfx::Rect work_area_bounds =
      GetDisplayWorkAreaBoundsInScreen(GetDefaultSnappedWindow());
  gfx::Point modified_location_in_screen =
      GetBoundedPosition(location_in_screen, work_area_bounds);

  // Update |divider_position_|.
  const int previous_divider_position = divider_position_;
  UpdateDividerPosition(modified_location_in_screen);

  // Restack windows order if necessary.
  RestackWindows(previous_divider_position, divider_position_);

  // Update the black scrim layer's bounds and opacity.
  UpdateBlackScrim(modified_location_in_screen);

  // Update the snapped window/windows and divider's position.
  UpdateSnappedWindowsAndDividerBounds();

  previous_event_location_ = modified_location_in_screen;
}

void SplitViewController::EndResize(const gfx::Point& location_in_screen) {
  DCHECK(IsSplitViewModeActive());
  // TODO(xdai): Use fade out animation instead of just removing it.
  black_scrim_layer_.reset();
  is_resizing_ = false;

  const gfx::Rect work_area_bounds =
      GetDisplayWorkAreaBoundsInScreen(GetDefaultSnappedWindow());
  gfx::Point modified_location_in_screen =
      GetBoundedPosition(location_in_screen, work_area_bounds);

  // Find the closest fixed point for |location_in_screen| and update the
  // |divider_position_|.
  float ratio = FindClosestFixedPositionRatio(
      modified_location_in_screen, work_area_bounds,
      IsCurrentScreenOrientationLandscape());
  switch (screen_orientation_) {
    case blink::kWebScreenOrientationLockLandscapePrimary:
      divider_position_ = work_area_bounds.width() * ratio;
      break;
    case blink::kWebScreenOrientationLockPortraitPrimary:
      divider_position_ = work_area_bounds.height() * (1.f - ratio);
      break;
    case blink::kWebScreenOrientationLockLandscapeSecondary:
      divider_position_ = work_area_bounds.width() * (1.f - ratio);
      break;
    case blink::kWebScreenOrientationLockPortraitSecondary:
      divider_position_ = work_area_bounds.height() * ratio;
      break;
    default:
      NOTREACHED();
      break;
  }

  // Check if one of the snapped windows needs to be closed.
  if (divider_position_ == 0 ||
      divider_position_ == (IsCurrentScreenOrientationLandscape()
                                ? work_area_bounds.width()
                                : work_area_bounds.height())) {
    aura::Window* active_window =
        (divider_position_ == 0) ? right_window_ : left_window_;
    if (active_window)
      wm::ActivateWindow(active_window);
    EndSplitView();
  } else {
    UpdateSnappedWindowsAndDividerBounds();
  }
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
  screen_orientation_ = blink::kWebScreenOrientationLockDefault;
  divider_position_ = -1;

  State previous_state = state_;
  state_ = NO_SNAP;
  NotifySplitViewStateChanged(previous_state, state_);

  Shell::Get()->NotifySplitViewModeEnded();
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
  if (default_snap_position_ == LEFT)
    SnapWindow(gained_active, SplitViewController::RIGHT);
  else if (default_snap_position_ == RIGHT)
    SnapWindow(gained_active, SplitViewController::LEFT);
}

void SplitViewController::OnOverviewModeStarting() {
  DCHECK(IsSplitViewModeActive());

  // If split view mode is active, reset |state_| to make it be able to select
  // another window from overview window grid.
  State previous_state = state_;
  if (default_snap_position_ == LEFT) {
    StopObserving(right_window_);
    right_window_ = nullptr;
    state_ = LEFT_SNAPPED;
  } else if (default_snap_position_ == RIGHT) {
    StopObserving(left_window_);
    left_window_ = nullptr;
    state_ = RIGHT_SNAPPED;
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
        if (default_snap_position_ == LEFT)
          SnapWindow(window, SplitViewController::RIGHT);
        else if (default_snap_position_ == RIGHT)
          SnapWindow(window, SplitViewController::LEFT);
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

  screen_orientation_ =
      Shell::Get()->screen_orientation_controller()->GetCurrentOrientation();
  UpdateSnappedWindowsAndDividerBounds();
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

int SplitViewController::GetDefaultDividerPosition(aura::Window* window) const {
  const gfx::Rect work_area_bounds_in_screen =
      GetDisplayWorkAreaBoundsInScreen(window);
  const gfx::Size divider_size = SplitViewDivider::GetDividerSize(
      work_area_bounds_in_screen, screen_orientation_, false /* is_dragging */);
  if (IsCurrentScreenOrientationLandscape())
    return (work_area_bounds_in_screen.width() - divider_size.width()) * 0.5f;
  else
    return (work_area_bounds_in_screen.height() - divider_size.height()) * 0.5f;
}

void SplitViewController::UpdateBlackScrim(
    const gfx::Point& location_in_screen) {
  DCHECK(IsSplitViewModeActive());

  if (!black_scrim_layer_) {
    // Create an invisible black scrim layer.
    black_scrim_layer_ = base::MakeUnique<ui::Layer>(ui::LAYER_SOLID_COLOR);
    black_scrim_layer_->SetColor(SK_ColorBLACK);
    GetDefaultSnappedWindow()->GetRootWindow()->layer()->Add(
        black_scrim_layer_.get());
    GetDefaultSnappedWindow()->GetRootWindow()->layer()->StackAtTop(
        black_scrim_layer_.get());
  }

  // Decide where the black scrim should show and update its bounds.
  SnapPosition position = GetBlackScrimPosition(location_in_screen);
  if (position == NONE) {
    black_scrim_layer_.reset();
    return;
  }
  black_scrim_layer_->SetBounds(
      GetSnappedWindowBoundsInScreen(GetDefaultSnappedWindow(), position));

  // Update its opacity. The opacity increases as it gets closer to the edge of
  // the screen.
  float opacity = 0.f;
  const gfx::Rect work_area_bounds =
      GetDisplayWorkAreaBoundsInScreen(GetDefaultSnappedWindow());
  if (IsCurrentScreenOrientationLandscape()) {
    int distance_x =
        std::min(std::abs(location_in_screen.x() - work_area_bounds.x()),
                 std::abs(work_area_bounds.right() - location_in_screen.x()));
    opacity = 1.f - float(distance_x) / float(work_area_bounds.width());
  } else {
    int distance_y =
        std::min(std::abs(location_in_screen.y() - work_area_bounds.y()),
                 std::abs(work_area_bounds.bottom() - location_in_screen.y()));
    opacity = 1.f - float(distance_y) / float(work_area_bounds.height());
  }
  black_scrim_layer_->SetOpacity(opacity);
}

void SplitViewController::RestackWindows(const int previous_divider_position,
                                         const int current_divider_position) {
  if (!left_window_ || !right_window_)
    return;
  DCHECK(IsSplitViewModeActive());
  DCHECK_EQ(left_window_->parent(), right_window_->parent());

  const int mid_position = GetDefaultDividerPosition(GetDefaultSnappedWindow());
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
  if (left_window_ && wm::GetWindowState(left_window_)->IsSnapped()) {
    const wm::WMEvent left_window_event(wm::WM_EVENT_SNAP_LEFT);
    wm::GetWindowState(left_window_)->OnWMEvent(&left_window_event);
  }
  if (right_window_ && wm::GetWindowState(right_window_)->IsSnapped()) {
    const wm::WMEvent right_window_event(wm::WM_EVENT_SNAP_RIGHT);
    wm::GetWindowState(right_window_)->OnWMEvent(&right_window_event);
  }

  // Update divider's bounds.
  split_view_divider_->UpdateDividerBounds(is_resizing_);
}

SplitViewController::SnapPosition SplitViewController::GetBlackScrimPosition(
    const gfx::Point& location_in_screen) {
  SnapPosition position = SplitViewController::NONE;
  const gfx::Rect work_area_bounds =
      GetDisplayWorkAreaBoundsInScreen(GetDefaultSnappedWindow());
  if (!work_area_bounds.Contains(location_in_screen))
    return position;

  switch (screen_orientation_) {
    case blink::kWebScreenOrientationLockLandscapePrimary:
      if (location_in_screen.x() <
          work_area_bounds.x() +
              work_area_bounds.width() * kFixedPositionRatios[1]) {
        position = LEFT;
      } else if (location_in_screen.x() >
                 work_area_bounds.x() +
                     work_area_bounds.width() * kFixedPositionRatios[3]) {
        position = RIGHT;
      }
      break;
    case blink::kWebScreenOrientationLockPortraitPrimary:
      if (location_in_screen.y() >
          work_area_bounds.y() +
              work_area_bounds.height() * kFixedPositionRatios[3]) {
        position = LEFT;
      } else if (location_in_screen.y() <
                 work_area_bounds.y() +
                     work_area_bounds.height() * kFixedPositionRatios[1]) {
        position = RIGHT;
      }
      break;
    case blink::kWebScreenOrientationLockLandscapeSecondary:
      if (location_in_screen.x() >
          work_area_bounds.x() +
              work_area_bounds.width() * kFixedPositionRatios[3]) {
        position = LEFT;
      } else if (location_in_screen.x() <
                 work_area_bounds.x() +
                     work_area_bounds.width() * kFixedPositionRatios[1]) {
        position = RIGHT;
      }
      break;
    case blink::kWebScreenOrientationLockPortraitSecondary:
      if (location_in_screen.y() <
          work_area_bounds.y() +
              work_area_bounds.height() * kFixedPositionRatios[1]) {
        position = LEFT;
      } else if (location_in_screen.y() >
                 work_area_bounds.y() +
                     work_area_bounds.height() * kFixedPositionRatios[3]) {
        position = RIGHT;
      }
      break;
    default:
      break;
  }
  return position;
}

void SplitViewController::UpdateDividerPosition(
    const gfx::Point& location_in_screen) {
  switch (screen_orientation_) {
    case blink::kWebScreenOrientationLockLandscapePrimary:
      divider_position_ +=
          location_in_screen.x() - previous_event_location_.x();
      break;
    case blink::kWebScreenOrientationLockPortraitPrimary:
      divider_position_ -=
          location_in_screen.y() - previous_event_location_.y();
      break;
    case blink::kWebScreenOrientationLockLandscapeSecondary:
      divider_position_ -=
          location_in_screen.x() - previous_event_location_.x();
      break;
    case blink::kWebScreenOrientationLockPortraitSecondary:
      divider_position_ +=
          location_in_screen.y() - previous_event_location_.y();
      break;
    default:
      break;
  }
  divider_position_ = (divider_position_ < 0) ? 0 : divider_position_;
}

}  // namespace ash
