// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_logger.h"

#include "base/timer/timer.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/system/devicetype.h"

namespace chromeos {
namespace power {
namespace ml {

UserActivityLogger::UserActivityLogger(
    UserActivityLoggerDelegate* const delegate,
    IdleEventNotifier* const idle_event_notifier,
    ui::UserActivityDetector* const detector,
    chromeos::PowerManagerClient* const power_manager_client,
    viz::mojom::VideoDetectorObserverRequest request)
    : logger_delegate_(delegate),
      idle_event_notifier_(idle_event_notifier),
      detector_(detector),
      power_manager_client_(power_manager_client),
      idle_event_observer_(this),
      binding_(this, std::move(request)) {
  DCHECK(idle_event_notifier_);
  idle_event_observer_.Add(idle_event_notifier_);

  DCHECK(detector_);
  detector_->AddObserver(this);

  DCHECK(power_manager_client_);
  power_manager_client_->AddObserver(this);
  power_manager_client_->RequestStatusUpdate();
  power_manager_client_->GetSwitchStates(base::BindOnce(
      &UserActivityLogger::OnReceiveSwitchStates, base::Unretained(this)));

  if (chromeos::GetDeviceType() == chromeos::DeviceType::kChromebook) {
    device_type_ = UserActivityEvent::Features::CHROMEBOOK;
  } else {
    device_type_ = UserActivityEvent::Features::UNKNOWN_DEVICE;
  }
}

UserActivityLogger::~UserActivityLogger() {
  DCHECK(power_manager_client_);
  power_manager_client_->RemoveObserver(this);

  DCHECK(detector_);
  detector_->RemoveObserver(this);
}

void UserActivityLogger::OnUserActivity(const ui::Event* /* event */) {
  MaybeLogEvent(UserActivityEvent::Event::REACTIVATE,
                UserActivityEvent::Event::USER_ACTIVITY);
}

void UserActivityLogger::LidEventReceived(
    chromeos::PowerManagerClient::LidState state,
    const base::TimeTicks& /* timestamp */) {
  lid_state_ = state;
  if (lid_state_ == chromeos::PowerManagerClient::LidState::CLOSED) {
    MaybeLogEvent(UserActivityEvent::Event::OFF,
                  UserActivityEvent::Event::LID_CLOSED);
  }
}

void UserActivityLogger::PowerChanged(
    const power_manager::PowerSupplyProperties& proto) {
  bool battery_status = (proto.external_power() ==
                         power_manager::PowerSupplyProperties::DISCONNECTED);

  bool power_source_changed = (battery_status != on_battery_);
  on_battery_ = battery_status;

  if (proto.has_battery_percent()) {
    battery_percent_ = proto.battery_percent();
  }
  // Only log when power source changed, don't care about percentage change.
  if (power_source_changed) {
    MaybeLogEvent(UserActivityEvent::Event::REACTIVATE,
                  UserActivityEvent::Event::POWER_CHANGED);
  }
}

void UserActivityLogger::TabletModeEventReceived(
    chromeos::PowerManagerClient::TabletMode mode,
    const base::TimeTicks& /* timestamp */) {
  tablet_mode_ = mode;
}

void UserActivityLogger::ScreenIdleStateChanged(
    const power_manager::ScreenIdleState& proto) {
  if (proto.dimmed()) {
    screen_idle_timer_.Start(
        FROM_HERE, idle_delay_,
        base::Bind(&UserActivityLogger::MaybeLogEvent, base::Unretained(this),
                   UserActivityEvent::Event::TIMEOUT,
                   UserActivityEvent::Event::IDLE_SLEEP));
  } else {
    screen_idle_timer_.Stop();
  }
}

void UserActivityLogger::OnVideoActivityStarted() {
  MaybeLogEvent(UserActivityEvent::Event::REACTIVATE,
                UserActivityEvent::Event::VIDEO_ACTIVITY);
}

void UserActivityLogger::OnIdleEventObserved(
    const IdleEventNotifier::ActivityData& activity_data) {
  idle_event_observed_ = true;
  ExtractFeatures(activity_data);
}

void UserActivityLogger::OnReceiveSwitchStates(
    base::Optional<chromeos::PowerManagerClient::SwitchStates> switch_states) {
  if (switch_states.has_value()) {
    lid_state_ = switch_states->lid_state;
    tablet_mode_ = switch_states->tablet_mode;
  }
}

void UserActivityLogger::ExtractFeatures(
    const IdleEventNotifier::ActivityData& activity_data) {
  features_.Clear();

  // Set time related features.
  features_.set_last_activity_time_sec(
      (activity_data.last_activity_time -
       activity_data.last_activity_time.LocalMidnight())
          .InSeconds());
  if (activity_data.last_user_activity_time.has_value()) {
    features_.set_last_user_activity_time_sec(
        (*activity_data.last_user_activity_time -
         activity_data.last_user_activity_time->LocalMidnight())
            .InSeconds());
  }

  base::Time::Exploded exploded;
  activity_data.last_activity_time.LocalExplode(&exploded);
  features_.set_last_activity_day(
      static_cast<chromeos::power::ml::UserActivityEvent_Features_DayOfWeek>(
          exploded.day_of_week));

  if (activity_data.last_mouse_time.has_value()) {
    features_.set_time_since_last_mouse_sec(
        (base::Time::Now() - *activity_data.last_mouse_time).InSeconds());
  }
  if (activity_data.last_key_time.has_value()) {
    features_.set_time_since_last_key_sec(
        (base::Time::Now() - *activity_data.last_key_time).InSeconds());
  }

  features_.set_recent_time_active_sec(
      (activity_data.last_activity_time - activity_data.earliest_activity_time)
          .InSeconds());

  // Set device mode.
  if (lid_state_ == chromeos::PowerManagerClient::LidState::CLOSED) {
    features_.set_device_mode(UserActivityEvent::Features::CLOSED_LID);
  } else if (lid_state_ == chromeos::PowerManagerClient::LidState::OPEN) {
    if (tablet_mode_ == chromeos::PowerManagerClient::TabletMode::ON) {
      features_.set_device_mode(UserActivityEvent::Features::TABLET);
    } else {
      features_.set_device_mode(UserActivityEvent::Features::CLAMSHELL);
    }
  } else {
    features_.set_device_mode(UserActivityEvent::Features::UNKNOWN_MODE);
  }

  features_.set_device_type(device_type_);

  if (battery_percent_.has_value()) {
    features_.set_battery_percent(*battery_percent_);
  }
  features_.set_on_battery(on_battery_);
}

void UserActivityLogger::MaybeLogEvent(
    UserActivityEvent::Event::Type type,
    UserActivityEvent::Event::Reason reason) {
  if (!idle_event_observed_)
    return;
  UserActivityEvent activity_event;

  UserActivityEvent::Event* event = activity_event.mutable_event();
  event->set_type(type);
  event->set_reason(reason);

  *activity_event.mutable_features() = features_;

  // Log to metrics.
  logger_delegate_->LogActivity(activity_event);
  idle_event_observed_ = false;
  screen_idle_timer_.Stop();
}

void UserActivityLogger::SetTaskRunnerForTesting(
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  screen_idle_timer_.SetTaskRunner(task_runner);
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
