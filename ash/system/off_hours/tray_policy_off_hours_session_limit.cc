// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/off_hours/tray_policy_off_hours_session_limit.h"

#include <algorithm>
#include <memory>

#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "base/time/time.h"

namespace ash {

TrayPolicyOffHoursSessionLimit::TrayPolicyOffHoursSessionLimit(
    SystemTray* system_tray)
    : TrayLimitedSession(system_tray) {
  Shell::Get()->session_controller()->AddObserver(this);
}

TrayPolicyOffHoursSessionLimit::~TrayPolicyOffHoursSessionLimit() {
  Shell::Get()->session_controller()->RemoveObserver(this);
}

void TrayPolicyOffHoursSessionLimit::OnPolicyOffHoursModeChanged() {
  Update();
}

bool TrayPolicyOffHoursSessionLimit::IsSessionStarted() {
  return Shell::Get()->session_controller()->policy_off_hours_mode();
}

base::TimeDelta TrayPolicyOffHoursSessionLimit::UpdateRemainingSessionTime() {
  SessionController* session = Shell::Get()->session_controller();
  base::TimeTicks off_hours_end_time = session->policy_off_hours_end_time();
  return std::max(off_hours_end_time - base::TimeTicks::Now(),
                  base::TimeDelta());
}

}  // namespace ash