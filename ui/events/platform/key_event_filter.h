// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_PLATFORM_KEY_EVENT_FILTER_H_
#define UI_EVENTS_PLATFORM_KEY_EVENT_FILTER_H_

namespace ui {

class KeyEvent;

// An interface to receive and process key events.  If an event is handled, then
// it should not be passed on to additional event handlers (i.e. it should be
// filtered).
class KeyEventFilter {
 public:
  KeyEventFilter() = default;
  virtual ~KeyEventFilter() = default;

  // Returns true if |event| was handled and should be filtered.
  virtual bool OnKeyEvent(const ui::KeyEvent& event) = 0;
};

}  // namespace ui

#endif  // UI_EVENTS_PLATFORM_KEY_EVENT_FILTER_H_
