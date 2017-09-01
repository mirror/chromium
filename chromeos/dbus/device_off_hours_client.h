// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DBUS_DEVICE_OFF_HOURS_CLIENT_H_
#define CHROME_BROWSER_CHROMEOS_DBUS_DEVICE_OFF_HOURS_CLIENT_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/dbus/dbus_client.h"
#include "dbus/bus.h"
#include "dbus/message.h"

namespace dbus {
class MethodCall;
}

namespace chromeos {

class DeviceOffHoursClient : public DBusClient {
 public:
  DeviceOffHoursClient();
  ~DeviceOffHoursClient() override;

  // Creates the instance.
  static DeviceOffHoursClient* Create();
  void Init(dbus::Bus* bus) override;

  bool IsNetworkSyncronized();

 private:
  void ServiceInitiallyAvailable(bool service_is_available);
  void GetSystemClockLastSyncInfo();
  void OnGotSystemClockLastSyncInfo(dbus::Response* response);

  dbus::ObjectProxy* system_clock_proxy_;
  base::WeakPtrFactory<DeviceOffHoursClient> weak_ptr_factory_;
  bool network_synchronized_ = false;

  DISALLOW_COPY_AND_ASSIGN(DeviceOffHoursClient);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DBUS_DEVICE_OFF_HOURS_CLIENT_H_