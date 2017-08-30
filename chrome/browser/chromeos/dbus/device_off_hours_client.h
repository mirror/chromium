// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DBUS_DEVICE_OFF_HOURS_CLIENT_H_
#define CHROME_BROWSER_CHROMEOS_DBUS_DEVICE_OFF_HOURS_CLIENT_H_

#include <stdint.h>

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/observer_list.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/dbus/dbus_client.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace dbus {
class MethodCall;
}

namespace chromeos {

class DeviceOffHoursClient : public DBusClient {
 public:
  DeviceOffHoursClient();
  ~DeviceOffHoursClient() override;

  void TimeUpdatedReceived(dbus::Signal* signal);
  void OnGotSystemClockLastSyncInfo(dbus::Response* response);
  void GetSystemClockLastSyncInfo();
  void ServiceInitiallyAvailable(bool service_is_available);
  void TimeUpdatedConnected(const std::string& interface_name,
                            const std::string& signal_name,
                            bool success);

 protected:
  void Init(dbus::Bus* bus) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceOffHoursClient);
  dbus::ObjectProxy* system_clock_proxy_;
  base::WeakPtrFactory<DeviceOffHoursClient> weak_ptr_factory_;
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DBUS_DEVICE_OFF_HOURS_CLIENT_H_
