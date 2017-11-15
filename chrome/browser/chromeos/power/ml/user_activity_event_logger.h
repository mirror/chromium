// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_EVENT_LOGGER_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_EVENT_LOGGER_H_

#include "base/timer/timer.h"
#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "chromeos/dbus/power_manager_client.h"

namespace chromeos {
namespace power {
namespace ml {

class UserActivityEventLogger : public PowerManagerClient::Observer {
 public:
  UserActivityEventLogger();

  // chromeos::PowerManagerClient::Observer overrides:
  void LidEventReceived(chromeos::PowerManagerClient::LidState state,
                        const base::TimeTicks& timestamp) override;
  void PowerChanged(const power_manager::PowerSupplyProperties& proto) override;

  void ScreenIdleStateChanged(
      const power_manager::ScreenIdleState& proto) override;

  void SuspendImminent(power_manager::SuspendImminent::Reason reason) override;

  void SuspendDone(const base::TimeDelta& sleep_duration) override;

 private:
  // Sets event_type_ and also event_type_found_ to true if logging_in_progress_
  // is true.
  void MaybeSetEventType(UserActivityEvent_Type type);

  // Resets model_start_timer, it's called when a new activity is observed.
  void ResetActivityTimer();

  // Starts logging. It sets logging_in_progress_ to true and extract features.
  void StartLogging();

  // Logs event if logging_in_progress_ is true. Features have been extracted,
  // only populate event_type_.
  void MaybeLogEvent();

  // Sets/resets model_timeout_timer_ to prepare logging of final event type.
  // Only called after we receive screen-off signal.
  void ScheduleDelayedTimeoutLogging();

  // TODO(jiameng): extract features and populate current_event_.
  void ExtractFeatures() {}
  // TODO(jiameng): send current_event_ to backend logs.
  void SendToLogs() {}

  bool logging_in_progress_;
  bool event_type_found_;
  UserActivityEvent_Type event_type_;
  UserActivityEvent current_event_;

  base::OneShotTimer model_start_timer_;
  base::OneShotTimer model_timeout_timer_;

  DISALLOW_COPY_AND_ASSIGN(UserActivityEventLogger);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_EVENT_LOGGER_H_
