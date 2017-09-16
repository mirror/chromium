// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/time/clock.h"
#include "base/time/time.h"
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
// This function is used by device_policy_decoder_chromeos to save "OffHours"
// policy in PolicyMap.
std::unique_ptr<base::DictionaryValue> ConvertOffHoursProtoToValue(
    const enterprise_management::DeviceOffHoursProto& container);

// Apply "OffHours" policy for proto which contains device policies. Return
// ChromeDeviceSettingsProto without |ignored_policies|. The system will revert
// to the default behavior for the removed policies.
std::unique_ptr<enterprise_management::ChromeDeviceSettingsProto>
ApplyOffHoursPolicyToProto(
    const enterprise_management::ChromeDeviceSettingsProto& input_policies);

// Return true if current user is allowed after "Offhours" mode ends and false
// otherwise.
bool IsCurrentUserAllowedBeyondOffHours();

// The main class for handling “OffHours” policy turns "OffHours" mode on and
// off, interacts with other classes using observers, handles server and client
// time, timezone.
//
// DeviceSettingService is owner of this object. Use to get a reference:
// DeviceSettingsService::Get()->device_off_hours_controller()
//
// Device setting proto is changed in DeviceSettingsService and doesn't contain
// ignored device policies from DeviceOffHoursProto during "OffHours" mode. It
// is changed exactly in DeviceSettingsService because it's late to change
// policies in PrefValueMap and PolicyMap. The system will revert to the default
// behavior for the removed policies. And behavior of policies is handled during
// decoding process from proto to PolicyMap.
class DeviceOffHoursController
    : public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  // Observer interface.
  class Observer {
   public:
    // Gets called when "OffHours" mode needs update because it could be changed
    // when current "OffHours" interval ends or next "OffHours" interval starts.
    // And "OffHours" policy need to get actual proto and be re-applied.
    virtual void OnOffHoursModeNeedsUpdate();

   protected:
    virtual ~Observer();
  };

  // Creates a device off hours controller instance.
  DeviceOffHoursController();
  DeviceOffHoursController(base::Clock* clock, base::TickClock* timer_clock);
  ~DeviceOffHoursController() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Return current "OffHours" mode status.
  bool IsOffHoursMode();

  // Check if "OffHours" mode is in current time. Update current status in
  // |off_hours_mode_|. Set timer to next update "OffHours" mode. Log out user
  // if "OffHours" mode is ended.
  void UpdateOffHoursMode(
      const enterprise_management::ChromeDeviceSettingsProto& input_policies);

 private:
  // Run OnOffHoursModeNeedsUpdate() for observers.
  void NotifyOffHoursModeNeedsUpdate() const;

  // net::NetworkChangeNotifier::NetworkChangeObserver:
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  // Turn on and off "OffHours" mode.
  void SetOffHoursMode(bool off_hours_enabled);

  // Timer for update device settings.
  void StartOffHoursTimer(base::TimeDelta delay);
  void StopOffHoursTimer();

  base::ObserverList<Observer> observers_;

  // Timer for update device settings at the begin of next “OffHours” interval
  // or at the end of current "OffHours" interval.
  std::unique_ptr<base::OneShotTimer> timer_;

  // TODO(yakovleva): description
  std::unique_ptr<base::Clock> clock_;

  bool off_hours_mode_ = false;

  DISALLOW_COPY_AND_ASSIGN(DeviceOffHoursController);
};
}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_
