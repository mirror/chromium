// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/keyboard_lock/keyboard_lock_host.h"

#include <X11/XKBlib.h>

#include "ui/gfx/x/x11_types.h"

namespace keyboard_lock {

// static
ui::KeyboardCode KeyboardLockHost::NativeKeycodeToKeyboardCode(
    uint32_t keycode) {
  return static_cast<ui::KeyboardCode>(XkbKeycodeToKeysym(
      gfx::GetXDisplay(), keycode, 0, 1));
}

}  // namespace keyboard_lock
