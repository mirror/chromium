// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/power/ml/idle_event_notifier.h"

#include "base/logging.h"
#include "base/time/default_clock.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager/idle.pb.h"
#include "chromeos/dbus/power_manager/suspend.pb.h"
#include "ui/base/user_activity/user_activity_detector.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"

namespace chromeos {
namespace power {
namespace ml {

IdleEventNotifier::ActivityData::ActivityData() {}
IdleEventNotifier::ActivityData::ActivityData(
    base::Time last_activity_time,
    base::Time earliest_activity_time,
    base::Optional<base::Time> last_user_activity_time,
    base::Optional<base::Time> last_mouse_time,
    base::Optional<base::Time> last_key_time)
    : last_activity_time(last_activity_time),
      earliest_activity_time(earliest_activity_time),
      last_user_activity_time(last_user_activity_time),
      last_mouse_time(last_mouse_time),
      last_key_time(last_key_time) {}

IdleEventNotifier::ActivityData::ActivityData(const ActivityData& input_data) {
  last_activity_time = input_data.last_activity_time;
  earliest_activity_time = input_data.earliest_activity_time;
  last_user_activity_time = input_data.last_user_activity_time;
  last_mouse_time = input_data.last_mouse_time;
  last_key_time = input_data.last_key_time;
}

IdleEventNotifier::IdleEventNotifier(const base::TimeDelta& idle_delay,
                                     ash::VideoDetector* video_detector)
    : idle_delay_(idle_delay),
      clock_(new base::DefaultClock()),
      video_detector_(video_detector) {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->AddObserver(
      this);
  // TODO(jiameng): this requires ui::UserActivityDetector to be running. Check
  // if we need to explicitly init it.
  ui::UserActivityDetector::Get()->AddObserver(this);
  LOG(ERROR) << "PASSED";
  if (!video_detector_) {
    LOG(WARNING)
        << "VideoDetector should be running to enable activity-based logging";
    video_state_ = ash::VideoDetector::State::NOT_PLAYING;
  } else {
    video_state_ = video_detector_->state();
    video_detector_->AddObserver(this);
  }
}

IdleEventNotifier::~IdleEventNotifier() {
  chromeos::DBusThreadManager::Get()->GetPowerManagerClient()->RemoveObserver(
      this);
  // TODO(jiameng): this requires ui::UserActivityDetector to be running.
  ui::UserActivityDetector::Get()->RemoveObserver(this);
  if (video_detector_)
    video_detector_->RemoveObserver(this);
}

void IdleEventNotifier::SetClockForTesting(
    std::unique_ptr<base::Clock> test_clock) {
  clock_ = std::move(test_clock);
}

void IdleEventNotifier::AddObserver(Observer* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void IdleEventNotifier::RemoveObserver(Observer* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void IdleEventNotifier::LidEventReceived(
    chromeos::PowerManagerClient::LidState state,
    const base::TimeTicks& /* timestamp */) {
  // Ignore lid-close event, as we will observe suspend signal.
  if (state == chromeos::PowerManagerClient::LidState::OPEN) {
    // Lid-open event is a user activity event (USER_OTHER), update user
    // activity data and timestamps.
    UpdateActivityData(clock_->Now(), ActivityType::USER_OTHER);
    ResetIdleDelayTimer();
  }
}

void IdleEventNotifier::PowerChanged(
    const power_manager::PowerSupplyProperties& proto) {
  if (external_power_ != proto.external_power()) {
    external_power_ = proto.external_power();
    // Power change is a user activity (USER_OTHER), update user activity data
    // and reset timer.
    UpdateActivityData(clock_->Now(), ActivityType::USER_OTHER);
    ResetIdleDelayTimer();
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
  // SuspendDone is triggered by user opening the lid (or other user
  // activities), update user activity data and reset timer.
  UpdateActivityData(clock_->Now(), ActivityType::USER_OTHER);
  ResetIdleDelayTimer();
}

void IdleEventNotifier::OnUserActivity(const ui::Event* event) {
  base::Time now = clock_->Now();
  // Get the type of activity first then reset timer.
  ActivityType type = ActivityType::USER_OTHER;
  if (event->IsKeyEvent()) {
    type = ActivityType::KEY;
  } else if (event->IsMouseEvent() || event->IsTouchEvent()) {
    // Treat mouse and touch events the same.
    type = ActivityType::MOUSE;
  }
  UpdateActivityData(now, type);
  ResetIdleDelayTimer();
}

void IdleEventNotifier::OnVideoStateChanged(ash::VideoDetector::State state) {
  video_state_ = state;
  if (video_state_ != ash::VideoDetector::State::NOT_PLAYING) {
    // Update last video time if it's still playing.
    base::Time now = clock_->Now();
    UpdateActivityData(now, ActivityType::VIDEO);
  }
}

void IdleEventNotifier::ResetIdleDelayTimer() {
  idle_delay_timer_.Start(FROM_HERE, idle_delay_, this,
                          &IdleEventNotifier::OnIdleDelayTimeout);
}

void IdleEventNotifier::OnIdleDelayTimeout() {
  // Video is playing, do not trigger idle event.
  if (video_state_ != ash::VideoDetector::State::NOT_PLAYING)
    return;

  DCHECK(earliest_activity_time_);
  // Generate an activity data.
  ActivityData activity_data(
      last_activity_time_, earliest_activity_time_.value(),
      last_user_activity_time_, last_mouse_time_, last_key_time_);

  // Reset earlist_active_time only when we're about to generate an idle event.
  earliest_activity_time_ = base::nullopt;

  for (auto& observer : observers_)
    observer.OnIdleEventObserved(activity_data);
}

void IdleEventNotifier::UpdateActivityData(const base::Time& now,
                                           ActivityType type) {
  // Always update last activity time.
  last_activity_time_ = now;

  if (!earliest_activity_time_) {
    earliest_activity_time_ = now;
  }

  if (type == ActivityType::VIDEO) {
    last_video_activity_time_ = now;
    return;
  }

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
