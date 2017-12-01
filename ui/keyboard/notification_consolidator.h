// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_NOTIFICATION_CONSOLIDATOR_H_
#define UI_KEYBOARD_NOTIFICATION_CONSOLIDATOR_H_

#include "ui/gfx/geometry/rect.h"
#include "ui/keyboard/keyboard_export.h"

namespace keyboard {

template <typename T>
class ValueNotificationConsolidator {
 public:
  ValueNotificationConsolidator() {}

  bool ShouldSendNotification(const T new_value);

 private:
  bool never_sent_ = true;
  T value_;
};

// Logic for consolidating consecutive identical notifications from the
// KeyboardControllerObserver.
class KEYBOARD_EXPORT NotificationConsolidator {
 public:
  NotificationConsolidator();

  bool ShouldSendAvailabilityNotification(bool current_availability);

  bool ShouldSendVisualBoundsNotification(const gfx::Rect& new_bounds);

  bool ShouldSendOccludedBoundsNotification(const gfx::Rect& new_bounds);

  bool ShouldSendWorkspaceDisplacementBoundsNotification(
      const gfx::Rect& new_bounds);

 private:
  // ValueNotificationConsolidator uses == for comparison, but empty rectangles
  // ought to be considered equal regardless of location or non-zero dimensions.
  // This method will return a default empty (0,0,0,0) rectangle for any 0-area
  // rectangle, otherwise it returns the original rectangle, unmodified.
  const gfx::Rect CanonicalizeEmptyRectangles(const gfx::Rect rect) const;

  ValueNotificationConsolidator<bool> availability_;
  ValueNotificationConsolidator<gfx::Rect> visual_bounds_;
  ValueNotificationConsolidator<gfx::Rect> occluded_bounds_;
  ValueNotificationConsolidator<gfx::Rect> workspace_displaced_bounds_;
};

}  // namespace keyboard

#endif  // UI_KEYBOARD_NOTIFICATION_CONSOLIDATOR_H_
