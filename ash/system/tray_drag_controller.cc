// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/tray_drag_controller.h"

#include "ash/shell.h"
#include "ash/system/tray/tray_background_view.h"
#include "ash/wm/maximize_mode/maximize_mode_controller.h"

namespace ash {

bool TrayDragController::ProcessGestureEvent(const ui::GestureEvent& event,
                                             TrayBackgroundView* target_view,
                                             bool is_on_bubble) {
  target_view_ = target_view;
  if (!Shell::Get()
           ->maximize_mode_controller()
           ->IsMaximizeModeWindowManagerEnabled() ||
      !shelf_->IsHorizontalAlignment()) {
    return false;
  }

  is_on_bubble_ = is_on_bubble;
  if (event.type() == ui::ET_GESTURE_SCROLL_BEGIN)
    return StartGestureDrag(event);

  if (!target_view_->HasBubble() || !is_in_drag_) {
    return false;
  }

  if (event.type() == ui::ET_GESTURE_SCROLL_UPDATE) {
    UpdateGestureDrag(event);
    return true;
  }

  if (event.type() == ui::ET_GESTURE_SCROLL_END ||
      event.type() == ui::ET_SCROLL_FLING_START) {
    CompleteGestureDrag(event);
    return true;
  }

  // Unexpected event. Reset the drag state and close the bubble.
  is_in_drag_ = false;
  target_view_->CloseBubble();
  return false;
}

bool TrayDragController::StartGestureDrag(const ui::GestureEvent& gesture) {
  if (!is_on_bubble_) {
    // Dragging happens on the tray view. Close the bubble if there is already
    // one opened. And return false to let shelf handle the event.
    if (target_view_->HasBubble()) {
      target_view_->CloseBubble();
      return false;
    }

    // If the scroll sequence begins to scroll downward, return false so that
    // the event will instead by handled by the shelf.
    if (gesture.details().scroll_y_hint() > 0)
      return false;

    target_view_->ShowBubble();
  }

  is_in_drag_ = true;
  gesture_drag_amount_ = 0.f;
  tray_bubble_bounds_ =
      target_view_->GetBubbleView()->GetWidget()->GetWindowBoundsInScreen();
  UpdateBubbleBounds();
  return true;
}

void TrayDragController::UpdateGestureDrag(const ui::GestureEvent& gesture) {
  UpdateBubbleBounds();
  gesture_drag_amount_ += gesture.details().scroll_y();
}

void TrayDragController::CompleteGestureDrag(const ui::GestureEvent& gesture) {
  const bool hide_bubble = !ShouldShowBubbleAfterScrollSequence(gesture);
  gfx::Rect target_bounds = tray_bubble_bounds_;

  if (hide_bubble)
    target_bounds.set_y(shelf_->GetIdealBounds().y());

  target_view_->AnimateToTargetBounds(target_bounds, hide_bubble);
  is_in_drag_ = false;
}

void TrayDragController::UpdateBubbleBounds() {
  gfx::Rect bounds_on_location = tray_bubble_bounds_;
  int bounds_y =
      (is_on_bubble_ ? tray_bubble_bounds_.y() : shelf_->GetIdealBounds().y()) +
      gesture_drag_amount_;
  bounds_on_location.set_y(std::max(bounds_y, tray_bubble_bounds_.y()));
  target_view_->GetBubbleView()->GetWidget()->SetBounds(bounds_on_location);
}

bool TrayDragController::ShouldShowBubbleAfterScrollSequence(
    const ui::GestureEvent& sequence_end) {
  // If the scroll sequence terminates with a fling, show the bubble if the
  // fling was fast enough and in the correct direction.
  if (sequence_end.type() == ui::ET_SCROLL_FLING_START &&
      fabs(sequence_end.details().velocity_y()) > kFlingVelocity) {
    return sequence_end.details().velocity_y() < 0;
  }

  DCHECK(sequence_end.type() == ui::ET_GESTURE_SCROLL_END ||
         sequence_end.type() == ui::ET_SCROLL_FLING_START);

  // Keep the bubble's original state if the |gesture_drag_amount_| doesn't
  // exceed one-third of the bubble's height.
  if (is_on_bubble_)
    return gesture_drag_amount_ < tray_bubble_bounds_.height() / 3.0;
  return -gesture_drag_amount_ >= tray_bubble_bounds_.height() / 3.0;
}

}  // namespace ash
