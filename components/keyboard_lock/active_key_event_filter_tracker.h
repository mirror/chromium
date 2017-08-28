// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_ACTIVE_KEY_EVENT_FILTER_TRACKER_H_
#define COMPONENTS_KEYBOARD_LOCK_ACTIVE_KEY_EVENT_FILTER_TRACKER_H_

#include "base/threading/thread_checker.h"

namespace keyboard_lock {

class KeyEventFilter;

// Tracks the active KeyEventFilter.
// All public functions must be executed in one thread.
class ActiveKeyEventFilterTracker final {
 public:
  ActiveKeyEventFilterTracker();
  ~ActiveKeyEventFilterTracker();

  void set(KeyEventFilter* filter);
  // If |filter| == |filter_|, set |filter_| to nullptr. It ensures no matter
  // whether LostFocus or GotFocus is fired first, we can always get the latest
  // active filter.
  void erase(const KeyEventFilter* filter);
  KeyEventFilter* get() const;

 private:
  KeyEventFilter* filter_ = nullptr;
  base::ThreadChecker thread_checker_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_ACTIVE_KEY_EVENT_FILTER_TRACKER_H_
