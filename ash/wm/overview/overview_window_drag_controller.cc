// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/overview_window_drag_controller.h"

#include "ash/screen_util.h"
#include "ash/shell.h"
#include "ash/wm/overview/scoped_transform_overview_window.h"
#include "ash/wm/overview/window_selector.h"
#include "ash/wm/overview/window_selector_item.h"
#include "ash/wm/splitview/split_view_controller.h"
#include "ash/wm/window_positioning_utils.h"
#include "ash/wm/window_state.h"
#include "ash/wm/wm_event.h"
#include "ash/wm/workspace/phantom_window_controller.h"
#include "ui/aura/window.h"
#include "ui/wm/core/coordinate_conversion.h"

namespace ash {

namespace {

// The minimum offset that will be considered as a drag event.
const int kMinimiumDragOffset = 5;

// Snapping distance between the dragged window with the screen edge. It's
// useful especially for touch events.
const int kScreenEdgeInsetForDrag = 200;

}  // namespace

OverviewWindowDragController::OverviewWindowDragController(
    WindowSelector* window_selector)
    : window_selector_(window_selector),
      split_view_controller_(Shell::Get()->split_view_controller()) {}

OverviewWindowDragController::~OverviewWindowDragController() {}

void OverviewWindowDragController::InitiateDrag(
    WindowSelectorItem* item,
    const gfx::Point& location_in_screen) {
  initial_event_location_ = location_in_screen;
  previous_event_location_ = location_in_screen;
  item_ = item;
}

void OverviewWindowDragController::Drag(WindowSelectorItem* item,
                                        const gfx::Point& location_in_screen) {
  DCHECK(item_ == item);

  if (!did_move_ &&
      (std::abs(location_in_screen.x() - initial_event_location_.x()) <
           kMinimiumDragOffset ||
       std::abs(location_in_screen.y() - initial_event_location_.y()) <
           kMinimiumDragOffset)) {
    return;
  }
  did_move_ = true;

  // Updates the dragged |item_|'s board accordingly.
  gfx::Rect bounds(item_->target_bounds());
  bounds.set_x(bounds.x() + location_in_screen.x() -
               previous_event_location_.x());
  bounds.set_y(bounds.y() + location_in_screen.y() -
               previous_event_location_.y());
  item_->SetBounds(bounds, OverviewAnimationType::OVERVIEW_ANIMATION_NONE);
  previous_event_location_ = location_in_screen;

  // Then updates the phantom window if applicable.
  UpdatePhantomWindow(location_in_screen);
}

void OverviewWindowDragController::CompleteDrag(WindowSelectorItem* item) {
  DCHECK(item_ == item);

  phantom_window_controller_.reset();

  if (!did_move_) {
    // If no drag was initiated (e.g., a click/tap on the overview window),
    // activate the window. If the split view is active and has a left window,
    // snap the current window to right. If the split view is active and has a
    // right window, snap the current window to left.
    SplitViewController::State split_state = split_view_controller_->state();
    if (split_state == SplitViewController::NOSNAP)
      window_selector_->SelectWindow(item);
    else if (split_state == SplitViewController::LEFT_SNAPPED)
      SnapToRight();
    else if (split_state == SplitViewController::RIGHT_SNAPPED)
      SnapToLeft();
  } else {
    // If the window was dragged around but should not be snapped, move it back
    // to overview window grid.
    if (snap_type_ == SNAP_NONE)
      window_selector_->PositionWindows(true /* animate */);
    else if (snap_type_ == SNAP_LEFT)
      SnapToLeft();
    else
      SnapToRight();
    did_move_ = false;
  }
}

void OverviewWindowDragController::UpdatePhantomWindow(
    const gfx::Point& location_in_screen) {
  SnapType last_snap_type = snap_type_;
  snap_type_ = GetSnapType(location_in_screen);
  if (snap_type_ == SNAP_NONE || snap_type_ != last_snap_type) {
    phantom_window_controller_.reset();
    if (snap_type_ == SNAP_NONE)
      return;
  }

  const bool can_snap = snap_type_ != SNAP_NONE &&
                        wm::GetWindowState(item_->GetWindow())->CanSnap();
  if (!can_snap) {
    snap_type_ = SNAP_NONE;
    phantom_window_controller_.reset();
    return;
  }

  aura::Window* target_window = item_->GetWindow();
  gfx::Rect phantom_bounds_in_screen =
      (snap_type_ == SNAP_LEFT)
          ? Shell::Get()->split_view_controller()->GetLeftWindowBoundsInScreen(
                target_window)
          : Shell::Get()->split_view_controller()->GetRightWindowBoundsInScreen(
                target_window);

  if (!phantom_window_controller_) {
    phantom_window_controller_.reset(
        new PhantomWindowController(target_window));
  }
  phantom_window_controller_->Show(phantom_bounds_in_screen);
}

OverviewWindowDragController::SnapType
OverviewWindowDragController::GetSnapType(
    const gfx::Point& location_in_screen) {
  gfx::Rect area(
      ScreenUtil::GetDisplayWorkAreaBoundsInParent(item_->GetWindow()));
  ::wm::ConvertRectToScreen(item_->GetWindow()->GetRootWindow(), &area);
  area.Inset(kScreenEdgeInsetForDrag, 0, kScreenEdgeInsetForDrag, 0);

  if (location_in_screen.x() <= area.x())
    return SNAP_LEFT;
  if (location_in_screen.x() >= area.right() - 1)
    return SNAP_RIGHT;
  return SNAP_NONE;
}

void OverviewWindowDragController::SnapToLeft() {
  item_->EnsureVisible();
  item_->RestoreWindow();

  // |item_| will be deleted after RemoveWindowSelectorItem().
  aura::Window* left_window = item_->GetWindow();
  window_selector_->RemoveWindowSelectorItem(item_);
  split_view_controller_->SetLeftWindow(left_window);
  item_ = nullptr;
}

void OverviewWindowDragController::SnapToRight() {
  item_->EnsureVisible();
  item_->RestoreWindow();

  // |item_| will be deleted after RemoveWindowSelectorItem().
  aura::Window* right_window = item_->GetWindow();
  window_selector_->RemoveWindowSelectorItem(item_);
  split_view_controller_->SetRightWindow(right_window);
  item_ = nullptr;
}

}  // namespace ash
