// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_IDLE_EVENT_NOTIFIER_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_IDLE_EVENT_NOTIFIER_H_

#include <memory>

#include "ash/wm/video_detector.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/dbus/power_manager_client.h"
#include "ui/base/user_activity/user_activity_observer.h"

namespace base {
class Clock;
}

namespace chromeos {
namespace power {
namespace ml {

// IdleEventNotifier listens to signals and notifies its observers when an idle
// event is generated. An idle event is generated when the idle period reaches
// |idle_delay_|. No further idle events will be generated until user becomes
// active again, followed by an idle period of |idle_delay_|.
class IdleEventNotifier : public PowerManagerClient::Observer,
                          public ui::UserActivityObserver,
                          public ash::VideoDetector::Observer {
 public:
  struct ActivityData {
    ActivityData();
    ActivityData(
        base::Time last_activity_time,
        base::Time earliest_activity_time,
        base::Optional<base::Time> last_user_activity_time = base::nullopt,
        base::Optional<base::Time> last_mouse_time = base::nullopt,
        base::Optional<base::Time> last_key_time = base::nullopt);

    ActivityData(const ActivityData& input_data);

    // Last activity time. This is the activity that triggers idle event.
    base::Time last_activity_time;
    base::Time earliest_activity_time;
    // Last user activity time, which could be different from
    // |last_activity_time| if the last activity is not a user activity (e.g.
    // video). It is also optional because there could be no user activity
    // before the idle event is fired.
    base::Optional<base::Time> last_user_activity_time;
    // Last mouse/key event time. Optional because there could be no mouse/key
    // activities before the idle event.
    base::Optional<base::Time> last_mouse_time;
    base::Optional<base::Time> last_key_time;
  };

  enum class ActivityType {
    USER_OTHER = 0,  // All other user-related activities.
    KEY = 1,
    MOUSE = 2,
    VIDEO = 3,
  };

  class Observer {
   public:
    // Called when an idle event is observed.
    virtual void OnIdleEventObserved(const ActivityData& activity_data) = 0;

   protected:
    virtual ~Observer() {}
  };

  IdleEventNotifier(const base::TimeDelta& idle_delay,
                    ash::VideoDetector* video_detector);
  ~IdleEventNotifier() override;

  // Set test clock so that we can check activity time.
  void SetClockForTesting(std::unique_ptr<base::Clock> test_clock);

  // Adds or removes an observer.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // chromeos::PowerManagerClient::Observer overrides:
  void LidEventReceived(chromeos::PowerManagerClient::LidState state,
                        const base::TimeTicks& timestamp) override;
  void PowerChanged(const power_manager::PowerSupplyProperties& proto) override;
  void SuspendImminent(power_manager::SuspendImminent::Reason reason) override;
  void SuspendDone(const base::TimeDelta& sleep_duration) override;

  // ui::UserActivityObserver overrides:
  void OnUserActivity(const ui::Event* event) override;

  // ash::VideoDetector::Observer overrides:
  void OnVideoStateChanged(ash::VideoDetector::State state) override;

  const base::TimeDelta& idle_delay() const { return idle_delay_; }
  void set_idle_delay(const base::TimeDelta& delay) { idle_delay_ = delay; }

 private:
  FRIEND_TEST_ALL_PREFIXES(IdleEventNotifierTest, CheckInitialValues);
  friend class IdleEventNotifierTest;

  // Resets |idle_delay_timer_|, it's called when a new activity is observed.
  void ResetIdleDelayTimer();
  // Called when |idle_delay_timer_| expires.
  void OnIdleDelayTimeout();
  // Update all activity related time stamps.
  void UpdateActivityData(const base::Time& now, ActivityType type);

  base::TimeDelta idle_delay_;

  // It is base::DefaultClock, but will be set to a mock clock for tests.
  std::unique_ptr<base::Clock> clock_;
  base::OneShotTimer idle_delay_timer_;

  // Last-received external power state. Changes are treated as user activity.
  base::Optional<power_manager::PowerSupplyProperties_ExternalPower>
      external_power_;

  base::ObserverList<Observer> observers_;

  // Fields corresponding to ActivityData.
  // Time of last activity, which may be a user or non-user activity (e.g.
  // video).
  base::Time last_activity_time_;
  base::Optional<base::Time> earliest_activity_time_;
  base::Optional<base::Time> last_user_activity_time_;
  base::Optional<base::Time> last_video_activity_time_;
  base::Optional<base::Time> last_mouse_time_;
  base::Optional<base::Time> last_key_time_;

  ash::VideoDetector* video_detector_;  // not owned

  // Most-recently-observed video state.
  // TODO(jiameng): this variable will be updated by OnVideoStateChanged and
  // also read by ResetIdleDelayTimer, consider whether to put a mutex around
  // it.
  ash::VideoDetector::State video_state_;

  DISALLOW_COPY_AND_ASSIGN(IdleEventNotifier);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_IDLE_EVENT_NOTIFIER_H_
