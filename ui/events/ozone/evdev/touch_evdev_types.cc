// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/events/ozone/evdev/touch_evdev_types.h"

namespace ui {

InProgressTouchEvdev::InProgressTouchEvdev() {
}

InProgressTouchEvdev::InProgressTouchEvdev(const InProgressTouchEvdev& other) =
    default;

InProgressTouchEvdev::~InProgressTouchEvdev() {}

void InProgressTouchEvdev::SetCancelReason(TouchCancelReason reason) {
  cancelled |= reason;
}
void InProgressTouchEvdev::ResetCancelReason(TouchCancelReason reason) {
  cancelled &= (~reason);
}
bool InProgressTouchEvdev::CheckCancelReason(TouchCancelReason reason) {
  return cancelled & reason;
}

}  // namespace ui
