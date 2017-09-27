// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/off_hours/tray_off_hours_session_limit.h"

#include <memory>

#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "base/time/time.h"

namespace ash {

TrayOffHoursSessionLimit::TrayOffHoursSessionLimit(SystemTray* system_tray)
    : TraySessionLengthLimit(system_tray) {}

void TrayOffHoursSessionLimit::OnOffHoursModeOrDurationChanged() {
  Update();
}

bool TrayOffHoursSessionLimit::IsSessionStarted() {
  return limit_state() != LimitState::LIMIT_NONE ||
         Shell::Get()->session_controller()->off_hours_mode();
}

void TrayOffHoursSessionLimit::UpdateState() {
  SessionController* session = Shell::Get()->session_controller();
  base::TimeTicks off_hours_end_time = session->off_hours_end_time();
  base::TimeDelta remaining_session_time =
      std::max(off_hours_end_time - base::TimeTicks::Now(), base::TimeDelta());
  bool off_hours_mode = session->off_hours_mode();
  if (off_hours_mode && !remaining_session_time.is_zero())
    UpdateTimer(remaining_session_time);
  else
    ResetState();
}

}  // namespace ash