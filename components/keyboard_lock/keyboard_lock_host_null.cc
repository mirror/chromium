// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/keyboard_lock_host.h"

namespace keyboard_lock {

// static
ui::KeyboardCode KeyboardLockHost::NativeKeycodeToKeyboardCode(
    uint32_t keycode) {
  return static_cast<ui::KeyboardCode>(keycode);
}

}  // namespace keyboard_lock
