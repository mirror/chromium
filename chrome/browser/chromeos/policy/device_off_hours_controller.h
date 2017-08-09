// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_
#define CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_

#include <memory>

#include "base/observer_list.h"
#include "base/strings/utf_string_conversions.h"
#include "base/timer/timer.h"
#include "chrome/browser/chromeos/policy/proto/chrome_device_policy.pb.h"
#include "chrome/browser/chromeos/system/timezone_util.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/session_manager_client.h"
#include "chromeos/settings/timezone_settings.h"
#include "components/policy/core/common/cloud/cloud_policy_store.h"
//#include "unicode/gregocal.h"

namespace policy {

class DeviceOffHoursController
    : public chromeos::system::TimezoneSettings::Observer,
      public chromeos::SessionManagerClient::Observer {
 public:
  class Observer {
   public:
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

  // Creates a device off hours controller instance.
  // TODO(yakovleva) where should call constructor?
  DeviceOffHoursController();
  ~DeviceOffHoursController() override;

  // Apply OffHours policy for device policies depends on intervals
  static std::unique_ptr<enterprise_management::ChromeDeviceSettingsProto>
  ApplyOffHoursMode(
      enterprise_management::ChromeDeviceSettingsProto* input_policies);
  static enterprise_management::ChromeDeviceSettingsProto clear_policies(
      enterprise_management::ChromeDeviceSettingsProto* input_policies,
      std::set<std::string> off_policies);

  // Timer, |delay| in seconds
  void StartOffHoursTimer(int delay);
  void StopOffHoursTimer();

 private:
  // system::TimezoneSettings::Observer
  void TimezoneChanged(const icu::TimeZone& timezone) override;
  base::ObserverList<Observer> observers_;
  // Run DeviceSettingsUpdated() for observers.
  void NotifyOffHoursModeChanged() const;
  // Timer
  base::OneShotTimer timer_;
  // SessionManagerClient
  void ScreenIsUnlocked() override;
};
}  // namespace policy

#endif  // CHROME_BROWSER_CHROMEOS_POLICY_DEVICE_OFF_HOURS_CONTROLLER_H_
