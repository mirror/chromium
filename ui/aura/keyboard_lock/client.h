// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_KEYBOARD_LOCK_CLIENT_H_
#define UI_AURA_KEYBOARD_LOCK_CLIENT_H_

#include "ui/events/event.h"

namespace ui {
namespace aura {
namespace keyboard_lock {

// A client of the keyboard lock feature. A client provides a callback to let
// its host deliver the key events to itself.
class Client {
 public:
  Client() = default;
  virtual ~Client() = default;

  virtual void OnKeyEvent(const ui::KeyEvent& event) = 0;
};

}  // namespace keyboard_lock
}  // namespace aura
}  // namespace ui

#endif  // UI_AURA_KEYBOARD_LOCK_CLIENT_H_
