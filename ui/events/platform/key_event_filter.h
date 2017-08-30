// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_PLATFORM_KEY_EVENT_FILTER_H_
#define UI_EVENTS_PLATFORM_KEY_EVENT_FILTER_H_

namespace ui {

class KeyEvent;

// An interface to receive and process a key event.
class KeyEventFilter {
 public:
  KeyEventFilter() = default;
  virtual ~KeyEventFilter() = default;

  // Consumers can call event->SetHandled() to stop the |event| from returning
  // to the OS.
  virtual void OnKeyEvent(ui::KeyEvent* event) = 0;
};

}  // namespace ui

#endif  // UI_EVENTS_PLATFORM_KEY_EVENT_FILTER_H_
