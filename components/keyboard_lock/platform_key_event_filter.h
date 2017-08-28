// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_PLATFORM_KEY_EVENT_FILTER_H_
#define COMPONENTS_KEYBOARD_LOCK_PLATFORM_KEY_EVENT_FILTER_H_

#include "ui/events/platform/platform_event_types.h"

namespace keyboard_lock {

class KeyEventFilter;

class PlatformKeyEventFilter final {
 public:
  PlatformKeyEventFilter(KeyEventFilter* filter);
  ~PlatformKeyEventFilter();

  bool OnPlatformEvent(const ui::PlatformEvent& event);

 private:
  KeyEventFilter* const filter_;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_PLATFORM_KEY_EVENT_FILTER_H_
