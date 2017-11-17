// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POWER_ML_IDLE_EVENT_NOTIFIER_H_
#define CHROME_BROWSER_CHROMEOS_POWER_ML_IDLE_EVENT_NOTIFIER_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "chromeos/dbus/power_manager/power_supply_properties.pb.h"
#include "chromeos/dbus/power_manager_client.h"

namespace chromeos {
namespace power {
namespace ml {

// IdleEventNotifier listens to signals and notifies its observers when an idle
// event is generated. An idle event is generated when the idle period reaches
// kIdleDelaySec. No further idle events will be generated until user
// becomes active again, followed by an idle period of kIdleDelaySec.
class IdleEventNotifier : public PowerManagerClient::Observer {
 public:
  struct ActivityData {
    // Last activity time.
    base::TimeTicks last_activity_time;
  };

  class Observer {
   public:
    // Called when an idle event is observed.
    virtual void OnIdleEventObserved(const ActivityData& activity_data) = 0;

   protected:
    virtual ~Observer() {}
  };

  static constexpr base::TimeDelta kDefaultModelStartDelay =
      base::TimeDelta::FromSeconds(10);

  // Sets the global instance. Must be called before any calls to Get().
  static void Initialize();

  // Destroys the global instance.
  static void Shutdown();

  // Returns true if the global instance is initialized.
  static bool IsInitialized();

  // Gets the global instance. Initialize must be called first.
  static IdleEventNotifier* Get();

  int GetIdleDelaySec();
  void SetIdleDelaySec(int delay);

  // Adds or removes an observer.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // chromeos::PowerManagerClient::Observer overrides:
  void LidEventReceived(chromeos::PowerManagerClient::LidState state,
                        const base::TimeTicks& timestamp) override;
  void PowerChanged(const power_manager::PowerSupplyProperties& proto) override;
  void SuspendImminent(power_manager::SuspendImminent::Reason reason) override;
  void SuspendDone(const base::TimeDelta& sleep_duration) override;

 protected:
  IdleEventNotifier();
  ~IdleEventNotifier() override;

 private:
  // Resets |idle_delay_timer_|, it's called when a new activity is observed.
  void ResetActivityTimer();
  // Called when |idle_delay_timer_| expires.
  void IdleEventObserved();

  // Default to kDefaultModelStartDelay.
  base::TimeDelta idle_delay_ = kDefaultModelStartDelay;

  // Timer used to fire idle event.
  base::OneShotTimer idle_delay_timer_;

  base::ObserverList<Observer> observers_;

  // Last-received external power state. Changes are treated as user activity.
  base::Optional<power_manager::PowerSupplyProperties_ExternalPower>
      external_power_;

  ActivityData activity_data_;

  friend class IdleEventNotifierTest;
  FRIEND_TEST_ALL_PREFIXES(IdleEventNotifierTest, CheckInitialValues);

  DISALLOW_COPY_AND_ASSIGN(IdleEventNotifier);
};

}  // namespace ml
}  // namespace power
}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_POWER_ML_IDLE_EVENT_NOTIFIER_H_
