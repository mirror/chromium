// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_SYSTEM_SESSION_TRAY_OFF_HOURS_SESSION_LIMIT_H_
#define ASH_SYSTEM_SESSION_TRAY_OFF_HOURS_SESSION_LIMIT_H_

#include <memory>

#include "ash/system/session/tray_session_length_limit.h"
#include "base/macros.h"

namespace ash {

// Notification during "OffHours" mode that shows
// remaining time in "OffHours" mode and reminds
// that user will log out when "OffHours" mode will ended.
// Notification can be hidden when remaining time
// is more than 5 minutes and can't be hidden after.
class ASH_EXPORT TrayOffHoursSessionLimit : public TraySessionLengthLimit {
 public:
  explicit TrayOffHoursSessionLimit(SystemTray* system_tray);

 private:
  // SessionObserver:
  // Called when "OffHours" policy mode or duration are changed.
  void OnOffHoursModeOrDurationChanged() override;

  // Return true if "OffHours" session is already started.
  bool IsSessionStarted() override;

  // Get "OffHours" policy session end time from session controller and update
  // remaining session time.
  void UpdateRemainingSessionTime() override;

  DISALLOW_COPY_AND_ASSIGN(TrayOffHoursSessionLimit);
};

}  // namespace ash

#endif  // ASH_SYSTEM_SESSION_TRAY_OFF_HOURS_SESSION_LIMIT_H_