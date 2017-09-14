// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_PLATFORM_PLATFORM_KEY_EVENT_FILTER_H_
#define UI_EVENTS_PLATFORM_PLATFORM_KEY_EVENT_FILTER_H_

#include "ui/events/platform/platform_event_types.h"

namespace ui {

class KeyEventFilter;

// An adapter to receive and forward a key event in ui::PlatformEvent format to
// a KeyEventFilter implementation.
class PlatformKeyEventFilter final {
 public:
  PlatformKeyEventFilter(KeyEventFilter* filter);
  ~PlatformKeyEventFilter();

  // Returns true if |event| has been consumed already.
  bool OnPlatformEvent(const ui::PlatformEvent& event);

 private:
  KeyEventFilter* const filter_;
};

}  // namespace ui

#endif  // UI_EVENTS_PLATFORM_PLATFORM_KEY_EVENT_FILTER_H_
