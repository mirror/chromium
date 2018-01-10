// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_PLATFORM_KEY_EVENT_FILTER_H_
#define UI_EVENTS_PLATFORM_KEY_EVENT_FILTER_H_

namespace ui {

class KeyEvent;

// An interface to receive and process key events.
class KeyEventFilter {
 public:
  KeyEventFilter() = default;
  virtual ~KeyEventFilter() = default;

  // Consumers can return true to stop the |event| from being handled by other
  // components.
  // On Windows, the LowLevelKeyboardProc calls this function through
  // PlatformKeyEventFilter. If the |event| is reserved by web page, it will be
  // consumed by web page directly. Otherwise LowLevelKeyboardProc returns
  // false, and the OS will send the same keyboard event to the regular input
  // pipeline, i.e. the WndProc on Windows.  Same goes for macOS.
  // On X11, the X11EventSource will call this function to decide whether
  // |event| was reserved by the web page. If |event| is not consumed,
  // X11EventSource will send the |event| to the regular input pipeline.
  virtual bool OnKeyEvent(const ui::KeyEvent& event) = 0;
};

}  // namespace ui

#endif  // UI_EVENTS_PLATFORM_KEY_EVENT_FILTER_H_
