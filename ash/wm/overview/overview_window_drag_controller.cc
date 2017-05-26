// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/overview/overview_window_drag_controller.h"

#include "ash/screen_util.h"
#include "ash/shell.h"
#include "ash/wm/overview/scoped_transform_overview_window.h"
#include "ash/wm/overview/window_grid.h"
#include "ash/wm/overview/window_selector.h"
#include "ash/wm/overview/window_selector_item.h"
#include "ash/wm/splitscreen/split_view_controller.h"
#include "ash/wm/window_positioning_utils.h"
#include "ash/wm/window_state.h"
#include "ash/wm/wm_event.h"
#include "ash/wm/workspace/phantom_window_controller.h"
#include "ash/wm_window.h"
#include "ui/aura/window.h"

namespace ash {

namespace {

// The minimum offset that will be considered as a drag event.
const int kMinimiumDragOffset = 5;

// Snapping distance between the dragged window with the screen edge. It's
// useful especially for touch events.
const int kScreenEdgeInsetForDrag = 20;

}  // namespace

OverviewWindowDragController::OverviewWindowDragController(
    WindowSelector* window_selector)
    : window_selector_(window_selector),
      split_view_controller_(Shell::Get()->split_view_controller()) {}

OverviewWindowDragController::~OverviewWindowDragController() {}

void OverviewWindowDragController::InitiateDrag(WindowSelectorItem* item,
                                                const ui::LocatedEvent& event) {
  initial_event_location_ = event.location();
}

void OverviewWindowDragController::Drag(WindowSelectorItem* item,
                                        const gfx::Point& point,
                                        int event_flags) {
  if (!did_move_ && (std::abs(point.x() - initial_event_location_.x()) <
                         kMinimiumDragOffset ||
                     std::abs(point.y() - initial_event_location_.y()) <
                         kMinimiumDragOffset)) {
    return;
  }
  did_move_ = true;

  // Take ownership of the dragged item.
  GetWindowSelectorItemFromGrid(item);
  DCHECK(item_.get() == item);

  // Updates the dragged |item_|'s board accordingly.
  gfx::Rect bounds(item_->target_bounds());
  bounds.set_x(bounds.x() + point.x() - initial_event_location_.x());
  bounds.set_y(bounds.y() + point.y() - initial_event_location_.y());
  item_->SetBounds(bounds, OverviewAnimationType::OVERVIEW_ANIMATION_NONE);

  // Then updates the phantom window if applicable.
  UpdatePhantomWindow(point);
}

void OverviewWindowDragController::CompleteDrag(WindowSelectorItem* item) {
  phantom_window_controller_.reset();

  if (!did_move_) {
    // Activate the window if no drag was initiated (i.e., a click/tap event
    // in the overview mode). If the split view is active and has a left window,
    // snap the current window to right. If the split view is active and has a
    // right window, snap the current window to left.
    SplitViewController::State split_state = split_view_controller_->state();
    if (split_state == SplitViewController::INACTIVE) {
      window_selector_->SelectWindow(item);
    } else if (split_state == SplitViewController::LEFT_ACTIVE) {
      GetWindowSelectorItemFromGrid(item);
      DCHECK(item_.get() == item);
      SnapToRight();
    } else if (split_state == SplitViewController::RIGHT_ACTIVE) {
      GetWindowSelectorItemFromGrid(item);
      DCHECK(item_.get() == item);
      SnapToLeft();
    }
  } else {
    if (snap_type_ == SNAP_NONE) {
      // Move the |item_| back to |grid_| and restore its bounds and position.
      grid_->InsertItem(std::move(item_), initial_position_in_grid_);
    } else if (snap_type_ == SNAP_LEFT) {
      SnapToLeft();
    } else {
      SnapToRight();
    }
    item_.reset();
    did_move_ = false;
  }
}

void OverviewWindowDragController::UpdatePhantomWindow(
    const gfx::Point& location) {
  gfx::Point target_origin = item_->target_bounds().origin();
  gfx::Point location_in_parent(location.x() + target_origin.x(),
                                location.y() + target_origin.y());

  SnapType last_snap_type = snap_type_;
  snap_type_ = GetSnapType(location_in_parent);
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
  const gfx::Rect phantom_bounds =
      (snap_type_ == SNAP_LEFT)
          ? wm::GetDefaultLeftSnappedWindowBoundsInParent(target_window)
          : wm::GetDefaultRightSnappedWindowBoundsInParent(target_window);

  if (!phantom_window_controller_) {
    phantom_window_controller_.reset(
        new PhantomWindowController(target_window));
  }
  phantom_window_controller_->Show(phantom_bounds);
}

OverviewWindowDragController::SnapType
OverviewWindowDragController::GetSnapType(const gfx::Point& location) {
  gfx::Rect area(
      ScreenUtil::GetDisplayWorkAreaBoundsInParent(item_->GetWindow()));
  area.Inset(kScreenEdgeInsetForDrag, 0, kScreenEdgeInsetForDrag, 0);
  if (location.x() <= area.x())
    return SNAP_LEFT;
  if (location.x() >= area.right() - 1)
    return SNAP_RIGHT;
  return SNAP_NONE;
}

void OverviewWindowDragController::GetWindowSelectorItemFromGrid(
    WindowSelectorItem* item) {
  for (auto& iter : window_selector_->grid_list()) {
    if (iter->Contains(item->GetWindow())) {
      grid_ = iter.get();
      item_ = iter->RemoveItem(item->GetWindow(), &initial_position_in_grid_);
      break;
    }
  }
}

void OverviewWindowDragController::SnapToLeft() {
  item_->EnsureVisible();
  item_->RestoreWindow();
  split_view_controller_->SetLeftWindow(item_->GetWindow());
}

void OverviewWindowDragController::SnapToRight() {
  item_->EnsureVisible();
  item_->RestoreWindow();
  split_view_controller_->SetRightWindow(item_->GetWindow());
}

}  // namespace ash
