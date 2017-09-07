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
#include "net/base/network_change_notifier.h"

namespace policy {

// Return DictionaryValue in format:
// { "timezone" : string,
//   "intervals" : list of Intervals,
//   "ignored_policies" : string list }
// Interval dictionary format:
// { "start" : WeeklyTime,
//   "end" : WeeklyTime }
// WeeklyTime dictionary format:
// { "day_of_week" : int # value is from 1 to 7 (1 = Monday, 2 = Tuesday, etc.)
//   "time" : int # in milliseconds from the beginning of the day.
// }
std::unique_ptr<base::DictionaryValue> ConvertOffHoursProtoToValue(
    const enterprise_management::DeviceOffHoursProto& container);

// Apply "OffHours" policy for device policies.
// Return ChromeDeviceSettingsProto without |ignored_policies|.
// |ignored_policies| will be apply with default values.
std::unique_ptr<enterprise_management::ChromeDeviceSettingsProto>
GetOffHoursProto(
    const enterprise_management::ChromeDeviceSettingsProto& input_policies);

// The main class for handling “OffHours” policy
// turns "OffHours" mode on and off,
// interacts with other classes using observers,
// handles server and client time, timezone.
class DeviceOffHoursController
    : public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  // Observer interface.
  class Observer {
   public:
    // Gets called when "OffHours" mode could be changed.
    virtual void OnOffHoursModeMaybeChanged();

   protected:
    virtual ~Observer();
  };

  // Add an observer.
  void AddObserver(Observer* observer);
  // Remove an observer.
  void RemoveObserver(Observer* observer);

  // Creates a device off hours controller instance.
  DeviceOffHoursController();
  ~DeviceOffHoursController();

  // Return current "OffHours" mode status.
  bool IsOffHoursMode();

  // Check if "OffHours" mode is in current time.
  // Update current status in |off_hours_mode_|.
  // Set timer to next update "OffHours" mode.
  // Log out user if "OffHours" mode is ended.
  void UpdateOffHoursMode(
      const enterprise_management::ChromeDeviceSettingsProto& input_policies);

 private:
  // Run OnOffHoursModeMaybeChanged() for observers.
  void NotifyOffHoursModeChanged() const;

  // net::NetworkChangeNotifier::NetworkChangeObserver:
  // Triggered when the device wakes up.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // Manage "OffHours" mode.
  // Return true if mode is set/unset.
  bool SetOffHoursMode();
  bool UnsetOffHoursMode();

  // Timer for update device settings.
  void StartOffHoursTimer(base::TimeDelta delay);
  void StopOffHoursTimer();

  base::ObserverList<Observer> observers_;

  // Timer for update device settings
  // at the begin of next “OffHours” interval
  // or at the end of current "OffHours" interval.
  base::OneShotTimer timer_;

  bool off_hours_mode_ = false;

  DISALLOW_COPY_AND_ASSIGN(DeviceOffHoursController);
};
}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_
