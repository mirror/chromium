// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_IDLE_EVENT_NOTIFIER_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_IDLE_EVENT_NOTIFIER_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/time/tick_clock.h"
#include "base/timer/timer.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/dbus/power_manager_client.h"
#include "ui/base/user_activity/user_activity_observer.h"

namespace chromeos {
namespace power {
namespace ml {

// IdleEventNotifier listens to signals and notifies its observers when an idle
// event is generated. An idle event is generated when the idle period reaches
// |idle_delay_sec|. No further idle events will be generated until user becomes
// active again, followed by an idle period of |idle_delay_sec|.
class IdleEventNotifier : public PowerManagerClient::Observer,
                          public ui::UserActivityObserver {
 public:
  struct ActivityData {
    ActivityData();
    ActivityData(const ActivityData& input_data);
    // Last activity time. This is the activity that triggers idle event.
    base::TimeTicks last_activity_time;
    // Last user activity time, which could be different from
    // |last_activity_time| if the last activity is not a user activity (e.g.
    // video). It is also optional because there could be no user activity
    // before the idle event is fired.
    base::Optional<base::TimeTicks> last_user_activity_time;

    // Last mouse/key event time. Optional because there could be no mouse/key
    // activities before the idle event.
    base::Optional<base::TimeTicks> last_mouse_time;
    base::Optional<base::TimeTicks> last_key_time;

    // Earliest active time (user or non-user activity, e.g. video). Optional
    // because there may be no activity before the last one.
    base::Optional<base::TimeTicks> earliest_active_time;
  };

  enum class ActivityType {
    USER_OTHER = 0,  // All other user-related activities.
    KEY = 1,
    MOUSE = 2,
    // TODO(jiameng): will add video.
  };

  class Observer {
   public:
    // Called when an idle event is observed.
    virtual void OnIdleEventObserved(const ActivityData& activity_data) = 0;

   protected:
    virtual ~Observer() {}
  };

  IdleEventNotifier(Observer* observer, int idle_delay_sec);
  ~IdleEventNotifier() override;

  void SetClockForTesting(std::unique_ptr<base::TickClock> test_tick_clock);

  int GetIdleDelaySec();
  void SetIdleDelaySec(int idle_delay_sec);

  // chromeos::PowerManagerClient::Observer overrides:
  void LidEventReceived(chromeos::PowerManagerClient::LidState state,
                        const base::TimeTicks& timestamp) override;
  void PowerChanged(const power_manager::PowerSupplyProperties& proto) override;
  void SuspendImminent(power_manager::SuspendImminent::Reason reason) override;
  void SuspendDone(const base::TimeDelta& sleep_duration) override;

  // ui::UserActivityObserver overrides:
  void OnUserActivity(const ui::Event* event) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(IdleEventNotifierTest, CheckInitialValues);
  friend class IdleEventNotifierTest;

  // Resets |idle_delay_timer_|, it's called when a new activity is observed.
  void ResetActivityTimer();
  // Called when |idle_delay_timer_| expires.
  void IdleEventObserved();
  // Update all activity related time stamps.
  void UpdateActivityData(const base::TimeTicks& now, ActivityType type);

  Observer* observer_;  // Not owned.
  base::TimeDelta idle_delay_;

  // Timer used to fire idle event.
  base::OneShotTimer idle_delay_timer_;

  // Last-received external power state. Changes are treated as user activity.
  base::Optional<power_manager::PowerSupplyProperties_ExternalPower>
      external_power_;

  // Fields corresponding to ActivityData.
  base::TimeTicks last_activity_time_;
  base::Optional<base::TimeTicks> last_user_activity_time_;
  base::Optional<base::TimeTicks> last_mouse_time_;
  base::Optional<base::TimeTicks> last_key_time_;
  base::Optional<base::TimeTicks> earliest_active_time_;

  // It is base::DefaultTickClock, but will be set to a mock tick clock for
  // tests.
  std::unique_ptr<base::TickClock> tick_clock_;
  DISALLOW_COPY_AND_ASSIGN(IdleEventNotifier);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_IDLE_EVENT_NOTIFIER_H_
