// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_POWER_POWER_BUTTON_CONTROLLER_UTIL_H_
#define ASH_SYSTEM_POWER_POWER_BUTTON_CONTROLLER_UTIL_H_

#include "ash/ash_export.h"
#include "base/time/time.h"

namespace power_button_controller_util {

// Ignore button-up events occurring within this many milliseconds of the
// previous button-up event. This prevents us from falling behind if the power
// button is pressed repeatedly.
static constexpr base::TimeDelta kIgnoreRepeatedButtonUpDelay =
    base::TimeDelta::FromMilliseconds(500);

// Amount of time since last screen state change that power button event needs
// to be ignored.
static constexpr base::TimeDelta kScreenStateChangeDelay =
    base::TimeDelta::FromMilliseconds(500);

// Amount of time since last SuspendDone() that power button event needs to be
// ignored.
static constexpr base::TimeDelta kIgnorePowerButtonAfterResumeDelay =
    base::TimeDelta::FromSeconds(2);

// Locks the screen if the "require password to wake from sleep" pref is set
// and locking is possible.
void LockScreenIfRequired();

}  // namespace power_button_controller_util

#endif  // ASH_SYSTEM_POWER_POWER_BUTTON_CONTROLLER_UTIL_H_
