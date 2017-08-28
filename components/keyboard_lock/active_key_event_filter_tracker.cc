// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/active_key_event_filter_tracker.h"

#include "base/logging.h"

namespace keyboard_lock {

ActiveKeyEventFilterTracker::ActiveKeyEventFilterTracker() = default;
ActiveKeyEventFilterTracker::~ActiveKeyEventFilterTracker() = default;

void ActiveKeyEventFilterTracker::set(KeyEventFilter* filter) {
  DCHECK(thread_checker_.CalledOnValidThread());
  filter_ = filter;
}

void ActiveKeyEventFilterTracker::erase(const KeyEventFilter* filter) {
  DCHECK(thread_checker_.CalledOnValidThread());
  if (filter_ == filter) {
    filter_ = nullptr;
  }
}

KeyEventFilter* ActiveKeyEventFilterTracker::get() const {
  DCHECK(thread_checker_.CalledOnValidThread());
  return filter_;
}

}  // namespace keyboard_lock
