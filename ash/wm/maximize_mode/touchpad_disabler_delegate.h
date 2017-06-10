// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WM_MAXIMIZE_MODE_TOUCHPAD_DISABLER_DELEGATE_H_
#define ASH_WM_MAXIMIZE_MODE_TOUCHPAD_DISABLER_DELEGATE_H_

#include "ash/ash_export.h"
#include "base/callback_forward.h"
#include "base/macros.h"

namespace ash {

// This class is an implementation detail of TouchpadDisabler and exists
// purely for testing.
//
// When TouchpadDisabler is created it calls DisableTouchpad(). The callback
// supplied to DisableTouchpad() is Run() with the result of disabling the
// touchpad. A return value of true indicates the touchpad was successfully
// disabled, false if it wasn't. If disabling the touchpad is unsuccessful,
// then no other calls are made to the delegate. If the callback supplies true,
// then DisableDependentState() is called immediately. Additionally, when the
// TouchpadDisabler is destroyed it calls EnableTouchpad(). If the callback
// supplies false, then DisableDependentState() and EnableTouchpad() are not
// called.
class ASH_EXPORT TouchpadDisablerDelegate {
 public:
  virtual ~TouchpadDisablerDelegate() {}

  using DisableResultClosure = base::OnceCallback<void(bool)>;
  virtual void DisableTouchpad(DisableResultClosure callback) = 0;

  virtual void DisableDependentState() = 0;

  virtual void EnableTouchpad() = 0;
};

}  // namespace ash

#endif  // ASH_WM_MAXIMIZE_MODE_TOUCHPAD_DISABLER_DELEGATE_H_
