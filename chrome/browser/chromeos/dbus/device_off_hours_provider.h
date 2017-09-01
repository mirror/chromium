// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_DBUS_DEVICE_OFF_HOURS_PROVIDER_H_
#define CHROME_BROWSER_CHROMEOS_DBUS_DEVICE_OFF_HOURS_PROVIDER_H_

#include <string>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "chromeos/dbus/services/cros_dbus_service.h"
#include "dbus/exported_object.h"

namespace dbus {
class MethodCall;
}

namespace chromeos {

class DeviceOffHoursProvider
    : public CrosDBusService::ServiceProviderInterface {
 public:
  DeviceOffHoursProvider();
  ~DeviceOffHoursProvider() override;

  // CrosDBusService::ServiceProviderInterface overrides:
  void Start(scoped_refptr<dbus::ExportedObject> exported_object) override;

 private:
  // Called from ExportedObject when a handler is exported as a D-Bus
  // method or failed to be exported.
  void OnExported(const std::string& interface_name,
                  const std::string& method_name,
                  bool success);

  // Called on UI thread in response to D-Bus requests.
  void IsOffHoursMode(dbus::MethodCall* method_call,
                      dbus::ExportedObject::ResponseSender response_sender);

  // Keep this last so that all weak pointers will be invalidated at the
  // beginning of destruction.
  base::WeakPtrFactory<DeviceOffHoursProvider> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DeviceOffHoursProvider);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_DBUS_DEVICE_OFF_HOURS_PROVIDER_H_