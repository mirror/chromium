// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_

#include <memory>

#include "base/observer_list.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"
#include "chromeos/dbus/session_manager_client.h"

namespace policy {

namespace off_hours {
// Return DictionaryValue in format:
// { "timezone" : string,
//   "intervals" : list of Intervals,
//   "ignored_policies" : string list }
// Interval dictionary format:
// { "start" : WeeklyTime,
//   "end" : WeeklyTime }
// WeeklyTime dictionary format:
// { "weekday" : int,
//   "time" : int }
std::unique_ptr<base::DictionaryValue> ConvertToValue(
    const enterprise_management::DeviceOffHoursProto& container);

}  // namespace off_hours

// The main class for handling “OffHours” policy
// turns "OffHours" mode on and off,
// interacts with other classes using observers,
// handles server and client time, timezone.
class DeviceOffHoursController
    : public chromeos::SessionManagerClient::Observer {
 public:
  // Observer interface.
  class Observer {
   public:
    // Gets called when "OffHours" mode is probably changed.
    virtual void OffHoursModeChanged();

    virtual void OffHoursControllerShutDown();

   protected:
    virtual ~Observer();
  };

  // Adds an observer.
  void AddObserver(Observer* observer);
  // Removes an observer.
  void RemoveObserver(Observer* observer);

  // Manage singleton instance.
  static void Initialize();
  static bool IsInitialized();
  static void Shutdown();
  static DeviceOffHoursController* Get();

  // Manage "OffHours" mode.
  bool IsOffHoursMode();
  void SetOffHoursMode();
  void UnsetOffHoursMode();

  // Timer for update device settings
  // at the begin of next “OffHours” interval
  // or at the end of current interval.
  // |delay| value in milliseconds.
  void StartOffHoursTimer(int delay);
  void StopOffHoursTimer();

  // Check "OffHours" mode.
  // Apply "OffHours" policy for device policies.
  static std::unique_ptr<enterprise_management::ChromeDeviceSettingsProto>
  ApplyOffHoursMode(
      enterprise_management::ChromeDeviceSettingsProto* input_policies);

 private:
  // Creates a device off hours controller instance.
  DeviceOffHoursController();
  ~DeviceOffHoursController();

  // Run DeviceSettingsUpdated() for observers.
  void NotifyOffHoursModeChanged() const;

  // SessionManagerClient
  void ScreenIsUnlocked() override;

  // Will log out user if user isn't eligible.
  void LogoutUserIfNeed();

  base::ObserverList<Observer> observers_;

  // Timer
  base::OneShotTimer timer_;

  bool off_hours_mode_ = false;
};
}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_
