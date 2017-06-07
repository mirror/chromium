// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_KEYBOARD_KEYBOARD_CONTROLLER_OBSERVER_H_
#define UI_KEYBOARD_KEYBOARD_CONTROLLER_OBSERVER_H_

#include "ui/keyboard/keyboard_controller.h"
#include "ui/keyboard/keyboard_export.h"

namespace gfx {
class Rect;
}

namespace keyboard {

// Observers to the KeyboardController are notified of significant events that
// occur with the keyboard, such as the bounds or visiility changing.
class KEYBOARD_EXPORT KeyboardControllerObserver {
 public:
  virtual ~KeyboardControllerObserver() {}

  // Called when the keyboard bounds or visibility are about to change.
  // TODO(oka): Consider replacing with OnStateChanged.
  virtual void OnKeyboardBoundsChanging(const gfx::Rect& new_bounds) = 0;

  // Called when the keyboard was closed. TODO(oka): Replace with
  // OnStateChanged.
  virtual void OnKeyboardClosed() = 0;

  // Called when the keyboard has been hidden and the hiding animation finished
  // successfully TODO(oka): Replace with OnStateChanged.
  virtual void OnKeyboardHidden() {}

  // When state changed.
  virtual void OnStateChanged(const KeyboardControllerState state) {}
};

}  // namespace keyboard

#endif  // UI_KEYBOARD_KEYBOARD_CONTROLLER_OBSERVER_H_
