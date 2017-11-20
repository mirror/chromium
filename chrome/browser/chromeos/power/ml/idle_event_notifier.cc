// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/idle_event_notifier.h"

#include "base/logging.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_manager/suspend.pb.h"

namespace chromeos {
namespace power {
namespace ml {

IdleEventNotifier::IdleEventNotifier(Observer* observer, int idle_delay_sec)
    : observer_(observer) {
  CHECK(observer_);
  idle_delay_ = base::TimeDelta::FromSeconds(idle_delay_sec);
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(
      this);
}

IdleEventNotifier::~IdleEventNotifier() {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(
      this);
}

int IdleEventNotifier::GetIdleDelaySec() {
  return idle_delay_.InSeconds();
}

void IdleEventNotifier::SetIdleDelaySec(int idle_delay_sec) {
  idle_delay_ = base::TimeDelta::FromSeconds(idle_delay_sec);
}

void IdleEventNotifier::LidEventReceived(
    chromeos::PowerManagerClient::LidState state,
    const base::TimeTicks& timestamp) {
  // Ignore lid-close event, as we will observe suspend signal.
  if (state == chromeos::PowerManagerClient::LidState::OPEN) {
    activity_data_.last_activity_time = timestamp;
    ResetActivityTimer();
  }
}

void IdleEventNotifier::PowerChanged(
    const power_manager::PowerSupplyProperties& proto) {
  if (external_power_ != proto.external_power()) {
    external_power_ = proto.external_power();
    activity_data_.last_activity_time = base::TimeTicks::Now();
    // Power change is a user activity, so reset timer.
    ResetActivityTimer();
  }
}

void IdleEventNotifier::SuspendImminent(
    power_manager::SuspendImminent::Reason reason) {
  if (idle_delay_timer_.IsRunning()) {
    // This means the user hasn't been idle for more than kModelStartDelaySec.
    // Regardless of the reason of suspend, we won't be emitting any idle event
    // now or following SuspendDone.
    // We stop the timer to prevent it from resuming in SuspendDone.
    idle_delay_timer_.Stop();
  }
}

void IdleEventNotifier::SuspendDone(const base::TimeDelta& sleep_duration) {
  // TODO(jiameng): consider check sleep_duration to decide whether to log
  // event.
  DCHECK(!idle_delay_timer_.IsRunning());
  activity_data_.last_activity_time = base::TimeTicks::Now();
  ResetActivityTimer();
}

void IdleEventNotifier::ResetActivityTimer() {
  idle_delay_timer_.Start(FROM_HERE, idle_delay_, this,
                          &IdleEventNotifier::IdleEventObserved);
}

void IdleEventNotifier::IdleEventObserved() {
  CHECK(observer_);
  observer_->OnIdleEventObserved(activity_data_);
  // TODO(jiameng): reset only some data in activity_data_ for the next event
  // when we add more fields in ActivityData.
  activity_data_ = {};
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
