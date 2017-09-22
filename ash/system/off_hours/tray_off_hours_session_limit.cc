// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/off_hours/tray_off_hours_session_limit.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "ash/resources/vector_icons/vector_icons.h"
#include "ash/session/session_controller.h"
#include "ash/shell.h"
#include "ash/strings/grit/ash_strings.h"
#include "ash/system/system_notifier.h"
#include "ash/system/tray/label_tray_view.h"
#include "ash/system/tray/system_tray.h"
#include "ash/system/tray/system_tray_notifier.h"
#include "ash/system/tray/tray_constants.h"
#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/time_format.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/message_center/message_center.h"
#include "ui/message_center/notification.h"
#include "ui/views/view.h"

namespace ash {

TrayOffHoursSessionLimit::TrayOffHoursSessionLimit(SystemTray* system_tray)
    : TraySessionLengthLimit(system_tray) {}

void TrayOffHoursSessionLimit::OnOffHoursModeChanged() {
  Update();
}

bool TrayOffHoursSessionLimit::IsSessionStarted() {
  return limit_state() != LimitState::LIMIT_NONE ||
         Shell::Get()->session_controller()->off_hours_mode();
}

void TrayOffHoursSessionLimit::UpdateState() {
  SessionController* session = Shell::Get()->session_controller();
  //  base::Time start_time = session->off_hours_start_time();
  base::TimeDelta duration = session->off_hours_duration();
  base::TimeTicks start_time = base::TimeTicks::Now();
  bool off_hours_mode = session->off_hours_mode();

  if (off_hours_mode && !duration.is_zero())
    UpdateTimer(duration, start_time);
  else
    ResetState();
}

}  // namespace ash