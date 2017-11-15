// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_event_logger.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_manager/suspend.pb.h"

namespace chromeos {
namespace power {
namespace ml {

const int kModelStartDelaySec = 10;
const int kModelTimeoutDelaySec = 10;

UserActivityEventLogger::UserActivityEventLogger()
    : logging_in_progress_(false), event_type_found_(false) {}

void UserActivityEventLogger::LidEventReceived(
    chromeos::PowerManagerClient::LidState state,
    const base::TimeTicks& timestamp) {
  // Ignore lid-close event, as we will observe suspend signal.
  if (state == chromeos::PowerManagerClient::LidState::OPEN) {
    ResetActivityTimer();
  }
}

void UserActivityEventLogger::PowerChanged(
    const power_manager::PowerSupplyProperties& proto) {
  SetEventType(UserActivityEvent_Type_REACTIVATE);
  MaybeLogEvent();
  ResetActivityTimer();
}

void UserActivityEventLogger::ScreenIdleStateChanged(
    const power_manager::ScreenIdleState& proto) {
  if (proto.off()) {
    // Start timeout timer to prepare final event logging.
    SetEventType(UserActivityEvent_Type_TIMEOUT);
    ScheduleDelayedTimeoutLogging();
  }
}

void UserActivityEventLogger::SuspendImminent(
    power_manager::SuspendImminent::Reason reason) {
  // We set event_type_ but only log in SuspendDone.
  switch (reason) {
    case power_manager::SuspendImminent_Reason_IDLE:
      SetEventType(UserActivityEvent_Type_TIMEOUT);
    case power_manager::SuspendImminent_Reason_LID_CLOSED:
      SetEventType(UserActivityEvent_Type_OFF);
    default:
      break;
  }
}

void UserActivityEventLogger::SuspendDone(
    const base::TimeDelta& sleep_duration) {
  // TODO(jiameng): consider check sleep_duration to decide whether to log
  // event.
  MaybeLogEvent();
  ResetActivityTimer();
}

void UserActivityEventLogger::SetEventType(UserActivityEvent_Type type) {
  event_type_ = type;
  event_type_found_ = true;
}

void UserActivityEventLogger::ResetActivityTimer() {
  CHECK(!logging_in_progress_);

  if (model_start_timer_.IsRunning()) {
    model_start_timer_.Reset();
  } else {
    model_start_timer_.Start(FROM_HERE,
                             base::TimeDelta::FromSeconds(kModelStartDelaySec),
                             this, &UserActivityEventLogger::StartLogging);
  }
}

void UserActivityEventLogger::StartLogging() {
  CHECK(!logging_in_progress_);
  logging_in_progress_ = true;
  ExtractFeatures();
}

void UserActivityEventLogger::MaybeLogEvent() {
  if (model_timeout_timer_.IsRunning()) {
    // No need to wait for timeout timer to end.
    model_timeout_timer_.Stop();
  }

  if (!logging_in_progress_) {
    return;
  }

  CHECK(event_type_found_);
  current_event_.set_type(event_type_);
  SendToLogs();
  current_event_.Clear();
  event_type_found_ = false;
  logging_in_progress_ = false;
}

void UserActivityEventLogger::ScheduleDelayedTimeoutLogging() {
  CHECK(!model_timeout_timer_.IsRunning());
  CHECK_EQ(event_type_, UserActivityEvent_Type_TIMEOUT);
  model_timeout_timer_.Start(
      FROM_HERE, base::TimeDelta::FromSeconds(kModelTimeoutDelaySec), this,
      &UserActivityEventLogger::MaybeLogEvent);
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
