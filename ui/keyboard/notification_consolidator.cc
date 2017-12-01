// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/keyboard/notification_consolidator.h"
#include "ui/gfx/geometry/rect.h"

namespace keyboard {

template <typename T>
bool ValueNotificationConsolidator<T>::ShouldSendNotification(
    const T new_value) {
  if (never_sent_) {
    value_ = new_value;
    never_sent_ = false;
    return true;
  }
  const bool value_changed = new_value != value_;
  if (value_changed) {
    value_ = new_value;
  }
  return value_changed;
}

NotificationConsolidator::NotificationConsolidator() {}

bool NotificationConsolidator::ShouldSendAvailabilityNotification(
    bool current_availability) {
  return availability_.ShouldSendNotification(current_availability);
}

bool NotificationConsolidator::ShouldSendVisualBoundsNotification(
    const gfx::Rect& new_bounds) {
  return visual_bounds_.ShouldSendNotification(
      CanonicalizeEmptyRectangles(new_bounds));
}

bool NotificationConsolidator::ShouldSendOccludedBoundsNotification(
    const gfx::Rect& new_bounds) {
  return occluded_bounds_.ShouldSendNotification(
      CanonicalizeEmptyRectangles(new_bounds));
}

bool NotificationConsolidator::
    ShouldSendWorkspaceDisplacementBoundsNotification(
        const gfx::Rect& new_bounds) {
  return workspace_displaced_bounds_.ShouldSendNotification(
      CanonicalizeEmptyRectangles(new_bounds));
}

const gfx::Rect NotificationConsolidator::CanonicalizeEmptyRectangles(
    const gfx::Rect rect) const {
  if (rect.width() == 0 || rect.height() == 0) {
    return gfx::Rect();
  }
  return rect;
}

}  // namespace keyboard
