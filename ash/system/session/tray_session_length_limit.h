// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_SESSION_TRAY_SESSION_LENGTH_LIMIT_H_
#define ASH_SYSTEM_SESSION_TRAY_SESSION_LENGTH_LIMIT_H_

#include <memory>

#include "ash/session/session_observer.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray_limited_session.h"
#include "base/macros.h"

namespace ash {

// Adds a countdown timer to the system tray if the session length is limited.
class ASH_EXPORT TraySessionLengthLimit : public TrayLimitedSession,
                                          public SessionObserver {
 public:
  explicit TraySessionLengthLimit(SystemTray* system_tray);
  ~TraySessionLengthLimit() override;

 private:
  friend class TraySessionLengthLimitTest;

  // SessionObserver:
  void OnSessionStateChanged(session_manager::SessionState state) override;
  void OnSessionLengthLimitChanged() override;

  bool IsSessionStarted() override;

  base::TimeDelta UpdateRemainingSessionTime() override;

  DISALLOW_COPY_AND_ASSIGN(TraySessionLengthLimit);
};

}  // namespace ash

#endif  // ASH_SYSTEM_SESSION_TRAY_SESSION_LENGTH_LIMIT_H_
