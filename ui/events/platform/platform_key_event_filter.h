// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_PLATFORM_PLATFORM_KEY_EVENT_FILTER_H_
#define UI_EVENTS_PLATFORM_PLATFORM_KEY_EVENT_FILTER_H_

#include "ui/events/platform/platform_event_types.h"

namespace ui {

class KeyEventFilter;

// An adapter to receive and process a key event in ui::PlatformEvent format in
// conjunction with an underlying KeyEventFilter implementation.  This class
// represents the shared logic for all implementations whereas
// |key_event_filter| can be platform or use-case specific.
class PlatformKeyEventFilter final {
 public:
  explicit PlatformKeyEventFilter(KeyEventFilter* key_event_filter);
  ~PlatformKeyEventFilter();

  // Returns true if |event| was handled and should not be propagated.
  bool OnPlatformEvent(const ui::PlatformEvent& event);

 private:
  KeyEventFilter* const key_event_filter_;
};

}  // namespace ui

#endif  // UI_EVENTS_PLATFORM_PLATFORM_KEY_EVENT_FILTER_H_
