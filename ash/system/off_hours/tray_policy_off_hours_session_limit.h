// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_OFF_HOURS_TRAY_POLICY_OFF_HOURS_SESSION_H_
#define ASH_SYSTEM_OFF_HOURS_TRAY_POLICY_OFF_HOURS_SESSION_H_

#include <memory>

#include "ash/session/session_observer.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray_limited_session.h"
#include "base/macros.h"

namespace ash {

// Notification during "OffHours" mode that shows remaining time in "OffHours"
// mode and reminds that user will log out when "OffHours" mode will ended.
// Notification can be hidden when remaining time is more than 5 minutes and
// can't be hidden after.
class ASH_EXPORT TrayPolicyOffHoursSessionLimit : public TrayLimitedSession,
                                                  public SessionObserver {
 public:
  explicit TrayPolicyOffHoursSessionLimit(SystemTray* system_tray);
  ~TrayPolicyOffHoursSessionLimit() override;

 private:
  // SessionObserver:
  void OnPolicyOffHoursModeChanged() override;

  // Return true if "OffHours" session is already started.
  bool IsSessionStarted() override;

  // Get "OffHours" policy session end time from session controller and return
  // actual remaining session time.
  base::TimeDelta UpdateRemainingSessionTime() override;

  DISALLOW_COPY_AND_ASSIGN(TrayPolicyOffHoursSessionLimit);
};

}  // namespace ash

#endif  // ASH_SYSTEM_OFF_HOURS_TRAY_POLICY_OFF_HOURS_SESSION_H_