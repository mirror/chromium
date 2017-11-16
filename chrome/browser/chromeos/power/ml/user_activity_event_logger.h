// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_EVENT_LOGGER_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_EVENT_LOGGER_H_

#include "base/macros.h"
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/dbus/power_manager_client.h"

namespace chromeos {
namespace power {
namespace ml {

// UserActivityEventLogger records events as UserActivityEvent and sends events
// to backend logs. After a certain period of inactivity, this class will
// extract features and will wait for signals to determine the appropriate event
// type. UserActivityEventLogger receives signals by implementing various
// interfaces. At any given time, at most one event can be logged.
class UserActivityEventLogger : public PowerManagerClient::Observer {
 public:
  UserActivityEventLogger();
  ~UserActivityEventLogger() override;

  // chromeos::PowerManagerClient::Observer overrides:
  void LidEventReceived(chromeos::PowerManagerClient::LidState state,
                        const base::TimeTicks& timestamp) override;
  void PowerChanged(const power_manager::PowerSupplyProperties& proto) override;
  void ScreenIdleStateChanged(
      const power_manager::ScreenIdleState& proto) override;
  void SuspendImminent(power_manager::SuspendImminent::Reason reason) override;
  void SuspendDone(const base::TimeDelta& sleep_duration) override;

 private:
  // Sets |event_type_| if |logging_in_progress_| is true.
  void MaybeSetEventType(UserActivityEvent_Type type);

  // Resets |model_start_timer_|, it's called when a new activity is observed.
  void ResetActivityTimer();

  // Starts logging. It sets |logging_in_progress_| to true.
  // TODO(jiameng): add feature extraction to this method.
  void StartLogging();

  // Logs event if |logging_in_progress_| is true. Features have been extracted,
  // only populate |event_type_|.
  void MaybeLogEvent();

  // Sets/resets |model_timeout_timer_| to prepare logging of final event type.
  // Only called after we receive screen-off signal.
  void ScheduleDelayedTimeoutLogging();

  // Only true if |model_start_timer_| has started and final event type is
  // unknown.
  bool logging_in_progress_ = false;

  // Final event type. Its value is base::nullopt if |logging_in_progress_| is
  // false or if event type is unknown. Its value may change while
  // |logging_in_progress_| is true. For example, its value would be set to
  // TIMEOUT after screen-off is observed but if while we're waiting for
  // |model_timeout_timer_| to expire, user reactivates, then final recorded
  // event type will be REACTIVATE.
  base::Optional<UserActivityEvent_Type> event_type_;

  // |current_event_| will be empty when |logging_in_progress_| is false. It
  // will be cleared after final logging is done (in MaybeLogEvent).
  UserActivityEvent current_event_;

  // Timers to start and time out logging. Delays are currently fixed as by
  // kModelStartDelaySec and kModelStartDelaySec.
  base::OneShotTimer model_start_timer_;
  base::OneShotTimer model_timeout_timer_;

  // External power. If this is changed, it will represent an user activity.
  base::Optional<power_manager::PowerSupplyProperties_ExternalPower>
      external_power_;

  DISALLOW_COPY_AND_ASSIGN(UserActivityEventLogger);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_USER_ACTIVITY_EVENT_LOGGER_H_
