// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_event_logger.h"
#include "base/logging.h"
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
    // logging_in_progress_ may be true if there was a SuspendImminent before
    // the lid is closed. As we can't be certain whether we would receive
    // LidState signal first or SuspendDone first, we have to prepare to log the
    // current event.
    MaybeLogEvent();
    ResetActivityTimer();
  }
}

void UserActivityEventLogger::PowerChanged(
    const power_manager::PowerSupplyProperties& proto) {
  // Set event_type_ and log event if logging_in_progress_ is true.
  MaybeSetEventType(UserActivityEvent_Type_REACTIVATE);
  MaybeLogEvent();
  // Power change is a user activity, so reset timer.
  ResetActivityTimer();
}

void UserActivityEventLogger::ScreenIdleStateChanged(
    const power_manager::ScreenIdleState& proto) {
  // We only need to observe screen-off signal.
  if (proto.off()) {
    // We should have started logging the current event now.
    CHECK(logging_in_progress_);
    // Start timeout timer to prepare final event logging.
    MaybeSetEventType(UserActivityEvent_Type_TIMEOUT);
    ScheduleDelayedTimeoutLogging();
  }
}

void UserActivityEventLogger::SuspendImminent(
    power_manager::SuspendImminent::Reason reason) {
  // We set event_type_ but only log in SuspendDone.
  switch (reason) {
    case power_manager::SuspendImminent_Reason_IDLE:
      CHECK(logging_in_progress_);
      MaybeSetEventType(UserActivityEvent_Type_TIMEOUT);
      break;
    case power_manager::SuspendImminent_Reason_LID_CLOSED:
    case power_manager::SuspendImminent_Reason_OTHER:
      MaybeSetEventType(UserActivityEvent_Type_OFF);
      break;
    default:
      NOTREACHED();
      break;
  }
}

void UserActivityEventLogger::SuspendDone(
    const base::TimeDelta& sleep_duration) {
  // TODO(jiameng): consider check sleep_duration to decide whether to log
  // event.
  MaybeLogEvent();
  // TODO(jiameng): if we receive both LidState::OPEN and SuspendDone, we should
  // really set activity timer once. But since we don't know which will come
  // first, we'll set it both times as the time diff should be small.
  ResetActivityTimer();
}

void UserActivityEventLogger::MaybeSetEventType(UserActivityEvent_Type type) {
  if (logging_in_progress_) {
    event_type_found_ = true;
    event_type_ = type;
  }
}

void UserActivityEventLogger::ResetActivityTimer() {
  // This function is only called after MaybeLogEvent, so logging_in_progress_
  // should be false.
  CHECK(!logging_in_progress_);

  if (model_start_timer_.IsRunning()) {
    model_start_timer_.Reset();
  } else {
    model_start_timer_.Start(FROM_HERE,
                             base::TimeDelta::FromSeconds(kModelStartDelaySec),
                             this, &UserActivityEventLogger::StartLogging);
  }

  // TODO(jiameng): record latest activity time as a feature.
}

void UserActivityEventLogger::StartLogging() {
  CHECK(!event_type_found_);
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

  // TIMEOUT may not be the final event type to be logged! Before timeout timer
  // expires, another user activity could be detected, and it will change event
  // type. System could be suspend, and the event type could be OFF.
  model_timeout_timer_.Start(
      FROM_HERE, base::TimeDelta::FromSeconds(kModelTimeoutDelaySec), this,
      &UserActivityEventLogger::MaybeLogEvent);
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
