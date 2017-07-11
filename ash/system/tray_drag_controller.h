// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_TRAY_DRAG_CONTROLLER_H_
#define ASH_SYSTEM_TRAY_DRAG_CONTROLLER_H_

#include "ash/ash_export.h"
#include "ash/shelf/shelf.h"
#include "ui/views/view.h"

namespace ash {
class TrayBackgroundView;

class ASH_EXPORT TrayDragController {
 public:
  // The threshold of the velocity of the fling event.
  static constexpr float kFlingVelocity = 100.0f;

  explicit TrayDragController(Shelf* shelf) : shelf_(shelf) {}

  // Process a gesture event and updates the dragging state of the bubble when
  // appropriate. Returns true if the gesture has been handled and it should not
  // be processed any further, false otherwise.
  bool ProcessGestureEvent(const ui::GestureEvent& event,
                           TrayBackgroundView* target_view,
                           bool is_on_bubble);

 private:
  // Gesture related functions:
  bool StartGestureDrag(const ui::GestureEvent& gesture);
  void UpdateGestureDrag(const ui::GestureEvent& gesture);
  void CompleteGestureDrag(const ui::GestureEvent& gesture);

  // Update the bounds of the tray bubbles according to
  // |gesture_drag_amount_|.
  void UpdateBubbleBounds();

  // Return true if the bubbles should be shown (i.e., animated upward to
  // be made fully visible) after a sequence of scroll events terminated by
  // |sequence_end|. Otherwise return false, indicating that the
  // partially-visible bubble should be animated downward and made fully
  // hidden.
  bool ShouldShowBubbleAfterScrollSequence(
      const ui::GestureEvent& sequence_end);

  Shelf* shelf_;
  TrayBackgroundView* target_view_ = nullptr;

  // The original bounds of the tray bubble.
  gfx::Rect tray_bubble_bounds_;

  // Tracks the amount of the drag. Only valid if |is_in_drag_| is true.
  float gesture_drag_amount_ = 0.f;

  // True if the user is in the process of gesture-dragging to open the system
  // tray bubble, false otherwise.
  bool is_in_drag_ = false;

  // True if the dragging happened on the bubble view, false if happened on the
  // tray view.
  bool is_on_bubble_ = false;

  DISALLOW_COPY_AND_ASSIGN(TrayDragController);
};

}  // namespace ash

#endif  // ASH_SYSTEM_TRAY_DRAG_CONTROLLER_H_
