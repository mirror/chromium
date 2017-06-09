// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/wm/maximize_mode/scoped_disable_internal_mouse_and_keyboard_ozone.h"

#include "ash/wm/maximize_mode/touchpad_disabler.h"
namespace ash {

ScopedDisableInternalMouseAndKeyboardOzone::
    ScopedDisableInternalMouseAndKeyboardOzone()
    : touchpad_disabler_(new TouchpadDisabler) {}

ScopedDisableInternalMouseAndKeyboardOzone::
    ~ScopedDisableInternalMouseAndKeyboardOzone() {
  touchpad_disabler_->Destroy();
}

}  // namespace ash
