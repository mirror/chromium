// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SHUTDOWN_REASON_H_
#define ASH_SHUTDOWN_REASON_H_

namespace ash {

enum class ShutdownReason {
  UNKNOWN,          // Reason unknown or not applicable
  POWER_BUTTON,     // User pressed the (physical) power button
  SIGNIN_SHUTDOWN,  // User pressed the signin screen shutdown button
  TRAY_SHUTDOWN,    // User pressed the tray shutdown button
};

}  // namespace ash

#endif  // ASH_SHUTDOWN_REASON_H_
