// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/active_key_event_filter.h"

#include "base/logging.h"
#include "components/keyboard_lock/active_key_event_filter_tracker.h"

namespace keyboard_lock {

ActiveKeyEventFilter::ActiveKeyEventFilter(
    const ActiveKeyEventFilterTracker* tracker)
    : tracker_(tracker) {
  DCHECK(tracker_);
}

ActiveKeyEventFilter::~ActiveKeyEventFilter() = default;

bool ActiveKeyEventFilter::OnKeyDown(ui::KeyboardCode code, int flags) {
  KeyEventFilter* filter = tracker_->get();
  if (!filter) {
    return false;
  }

  return filter->OnKeyDown(code, flags);
}

bool ActiveKeyEventFilter::OnKeyUp(ui::KeyboardCode code, int flags) {
  KeyEventFilter* filter = tracker_->get();
  if (!filter) {
    return false;
  }

  return filter->OnKeyUp(code, flags);
}

}  // namespace keyboard_lock
