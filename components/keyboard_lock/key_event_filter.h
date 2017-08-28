// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYBOARD_LOCK_KEY_EVENT_FILTER_H_
#define COMPONENTS_KEYBOARD_LOCK_KEY_EVENT_FILTER_H_

#include "ui/events/event_constants.h"
#include "ui/events/keycodes/keyboard_codes.h"

namespace keyboard_lock {

// An interface to receive and process a key event. The implementation needs to
// guarantee the thread-safety of OnKeyDown() and OnKeyUp() functions, which may
// be executed in a unpredictable thread.
class KeyEventFilter {
 public:
  KeyEventFilter() = default;
  virtual ~KeyEventFilter() = default;

  // Returns true if the |code| should be suppressed.
  virtual bool OnKeyDown(ui::KeyboardCode code, int flags) = 0;

  // REturns true if the |code| should be suppressed.
  virtual bool OnKeyUp(ui::KeyboardCode code, int flags) = 0;
};

}  // namespace keyboard_lock

#endif  // COMPONENTS_KEYBOARD_LOCK_KEY_EVENT_FILTER_H_
