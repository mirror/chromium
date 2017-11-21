// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/idle_event_notifier.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/time/default_tick_clock.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_manager/suspend.pb.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"

namespace chromeos {
namespace power {
namespace ml {

IdleEventNotifier::ActivityData::ActivityData() {}
IdleEventNotifier::ActivityData::ActivityData(const ActivityData& input_data) {
  last_activity_time = input_data.last_activity_time;
  last_user_activity_time = input_data.last_user_activity_time;
  last_mouse_time = input_data.last_mouse_time;
  last_key_time = input_data.last_key_time;
  earliest_active_time = input_data.earliest_active_time;
}

IdleEventNotifier::IdleEventNotifier(Observer* observer, int idle_delay_sec)
    : observer_(observer), tick_clock_(new base::DefaultTickClock()) {
  CHECK(observer_);
  idle_delay_ = base::TimeDelta::FromSeconds(idle_delay_sec);
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(
      this);
}

IdleEventNotifier::~IdleEventNotifier() {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(
      this);
}

void IdleEventNotifier::SetClockForTesting(
    std::unique_ptr<base::TickClock> test_tick_clock) {
  tick_clock_ = std::move(test_tick_clock);
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
    // Lid-open event is a user activity event (USER_OTHER), update user
    // activity data and timestamps.
    UpdateActivityData(timestamp, ActivityType::USER_OTHER);
    ResetActivityTimer();
  }
}

void IdleEventNotifier::PowerChanged(
    const power_manager::PowerSupplyProperties& proto) {
  if (external_power_ != proto.external_power()) {
    external_power_ = proto.external_power();
    base::TimeTicks now = tick_clock_->NowTicks();
    // Power change is a user activity (USER_OTHER), update user activity data
    // and reset timer.
    UpdateActivityData(now, ActivityType::USER_OTHER);
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
  base::TimeTicks now = tick_clock_->NowTicks();
  // SuspendDone is triggered by user opening the lid (or other user
  // activities), update user activity data and reset timer.
  UpdateActivityData(now, ActivityType::USER_OTHER);
  ResetActivityTimer();
}

void IdleEventNotifier::OnUserActivity(const ui::Event* event) {
  base::TimeTicks now = tick_clock_->NowTicks();
  // Get the type of activity first then reset timer.
  ActivityType type = ActivityType::USER_OTHER;
  if (event->IsKeyEvent()) {
    type = ActivityType::KEY;
  } else if (event->IsMouseEvent() || event->IsTouchEvent()) {
    // Treat mouse and touch events the same.
    type = ActivityType::MOUSE;
  }
  UpdateActivityData(now, type);
  ResetActivityTimer();
}

void IdleEventNotifier::ResetActivityTimer() {
  idle_delay_timer_.Start(FROM_HERE, idle_delay_, this,
                          &IdleEventNotifier::IdleEventObserved);
}

void IdleEventNotifier::IdleEventObserved() {
  CHECK(observer_);
  // Generate an activity data.
  ActivityData activity_data;
  activity_data.last_activity_time = last_activity_time_;
  activity_data.last_user_activity_time = last_user_activity_time_;
  activity_data.last_mouse_time = last_mouse_time_;
  activity_data.last_key_time = last_key_time_;
  activity_data.earliest_active_time = earliest_active_time_;

  // Reset earlist_active_time only when we're about to generate an idle event.
  earliest_active_time_ = last_activity_time_;

  observer_->OnIdleEventObserved(activity_data);
}

void IdleEventNotifier::UpdateActivityData(const base::TimeTicks& now,
                                           ActivityType type) {
  // Always update last activity time.
  last_activity_time_ = now;
  // At the moment, always update last user activity time, but this will change
  // once we introduce other signals, e.g. video activity.
  last_user_activity_time_ = now;
  if (type == ActivityType::USER_OTHER) {
    return;
  }
  if (type == ActivityType::KEY) {
    last_key_time_ = now;
    return;
  }
  last_mouse_time_ = now;
}

}  // namespace ml
}  // namespace power
}  // namespace chromeos
