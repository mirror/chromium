// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_PLATFORM_KEY_EVENT_FILTER_H_
#define UI_EVENTS_PLATFORM_KEY_EVENT_FILTER_H_

#include "ui/events/keycodes/keyboard_codes.h"

namespace ui {

// An interface to receive and process a key event in KeyboardCode and flags
// format.
class KeyEventFilter {
 public:
  KeyEventFilter() = default;
  virtual ~KeyEventFilter() = default;

  // Returns true if |code| has been consumed already.
  virtual bool OnKeyDown(ui::KeyboardCode code, int flags) = 0;

  // Returns true if |code| has been consumed already.
  virtual bool OnKeyUp(ui::KeyboardCode code, int flags) = 0;
};

}  // namespace ui

#endif  // UI_EVENTS_PLATFORM_KEY_EVENT_FILTER_H_
