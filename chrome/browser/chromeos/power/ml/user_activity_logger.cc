// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/user_activity_logger.h"

#include "base/timer/timer.h"
#include "chrome/browser/chromeos/power/ml/user_activity_event.pb.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "ui/base/user_activity/user_activity_observer.h"

namespace chromeos {
namespace power {
namespace ml {

UserActivityLogger::UserActivityLogger(
    UserActivityLoggerDelegate* const delegate)
    : logger_delegate_(delegate) {}

UserActivityLogger::~UserActivityLogger() = default;

// Only log when we observe an idle event.
void UserActivityLogger::OnUserActivity(const ui::Event* /* event */) {
  if (!idle_event_observed_)
    return;
  UserActivityEvent activity_event;

  UserActivityEvent::Event* event = activity_event.mutable_event();
  event->set_type(UserActivityEvent::Event::REACTIVATE);
  event->set_reason(UserActivityEvent::Event::USER_ACTIVITY);

  *activity_event.mutable_features() = ExtractFeatures();

  // Log to metrics.
  logger_delegate_->LogActivity(activity_event);
  idle_event_observed_ = false;
}

void UserActivityLogger::OnIdleEventObserved(
    const IdleEventNotifier::ActivityData& activity_data) {
  idle_event_observed_ = true;
  activity_data_ = activity_data;
}

void UserActivityLogger::LidEventReceived(
    chromeos::PowerManagerClient::LidState state,
    const base::TimeTicks& /* timestamp */) {
  lid_state_ = state;
}

void UserActivityLogger::PowerChanged(
    const power_manager::PowerSupplyProperties& proto) {
  if (proto.external_power() ==
      power_manager::PowerSupplyProperties::DISCONNECTED) {
    on_battery_ = true;
  } else {
    on_battery_ = false;
  }

  battery_percent_ = proto.battery_percent();
}

void UserActivityLogger::TabletModeEventReceived(
    chromeos::PowerManagerClient::TabletMode mode,
    const base::TimeTicks& /* timestamp */) {
  tablet_mode_ = mode;
}

UserActivityEvent::Features UserActivityLogger::ExtractFeatures() {
  UserActivityEvent::Features features;

  features.set_last_activity_time_sec(
      (activity_data_.last_activity_time -
       activity_data_.last_activity_time.LocalMidnight())
          .InSeconds());

  // Set device mode.
  if (lid_state_ == chromeos::PowerManagerClient::LidState::CLOSED) {
    features.set_device_mode(UserActivityEvent::Features::CLOSED_LID);
  } else if (lid_state_ == chromeos::PowerManagerClient::LidState::OPEN) {
    if (tablet_mode_ == chromeos::PowerManagerClient::TabletMode::ON) {
      features.set_device_mode(UserActivityEvent::Features::TABLET);
    } else {
      features.set_device_mode(UserActivityEvent::Features::CLAMSHELL);
    }
  } else {
    features.set_device_mode(UserActivityEvent::Features::UNKNOWN_MODE);
  }

  features.set_battery_percent(battery_percent_);
  features.set_on_battery(on_battery_);
  return features;
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
