// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/session/tray_session_length_limit.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "base/time/time.h"

namespace ash {

TraySessionLengthLimit::TraySessionLengthLimit(SystemTray* system_tray)
    : TrayLimitedSession(system_tray) {
  Shell::Get()->session_controller()->AddObserver(this);
}

TraySessionLengthLimit::~TraySessionLengthLimit() {
  Shell::Get()->session_controller()->RemoveObserver(this);
}

void TraySessionLengthLimit::OnSessionStateChanged(
    session_manager::SessionState state) {
  Update();
}

void TraySessionLengthLimit::OnSessionLengthLimitChanged() {
  Update();
}

bool TraySessionLengthLimit::IsSessionStarted() {
  // Don't show notification or tray item until the user is logged in.
  return Shell::Get()->session_controller()->IsActiveUserSessionStarted();
}

base::TimeDelta TraySessionLengthLimit::UpdateRemainingSessionTime() {
  SessionController* session = Shell::Get()->session_controller();
  base::TimeDelta time_limit = session->session_length_limit();
  base::TimeTicks session_start_time = session->session_start_time();
  return std::max(time_limit - (base::TimeTicks::Now() - session_start_time),
                  base::TimeDelta());
}

}  // namespace ash
