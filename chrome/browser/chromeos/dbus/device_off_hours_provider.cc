// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/dbus/device_off_hours_provider.h"

#include "base/bind.h"
#include "chrome/browser/chromeos/policy/device_off_hours_controller.h"
#include "chrome/browser/chromeos/settings/device_settings_service.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace em = enterprise_management;

namespace chromeos {

DeviceOffHoursProvider::DeviceOffHoursProvider() : weak_ptr_factory_(this) {}

DeviceOffHoursProvider::~DeviceOffHoursProvider() {}

void DeviceOffHoursProvider::Start(
    scoped_refptr<dbus::ExportedObject> exported_object) {
  LOG(ERROR) << "Daria start off hours Provider";
  // TODO(yakovleva): create keys
  exported_object->ExportMethod(
      system_clock::kSystemClockInterface, system_clock::kSystemLastSyncInfo,
      base::Bind(&DeviceOffHoursProvider::IsOffHoursMode,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&DeviceOffHoursProvider::OnExported,
                 weak_ptr_factory_.GetWeakPtr()));
}

void DeviceOffHoursProvider::OnExported(const std::string& interface_name,
                                        const std::string& method_name,
                                        bool success) {
  LOG(ERROR) << "Daria OnExported " << success;
  if (!success) {
    LOG(ERROR) << "Failed to export " << interface_name << "." << method_name;
  }
}

void DeviceOffHoursProvider::IsOffHoursMode(
    dbus::MethodCall* method_call,
    dbus::ExportedObject::ResponseSender response_sender) {
  LOG(ERROR) << "Daria IsOffHoursMode ? ";
  if (!method_call)
    return;
  std::unique_ptr<dbus::Response> response =
      dbus::Response::FromMethodCall(method_call);
  LOG(ERROR) << "Daria WE GOT MESSAGE " << response->ToString();

  const em::ChromeDeviceSettingsProto* device_settings =
      DeviceSettingsService::Get()->device_settings();

  em::ChromeDeviceSettingsProto policies;
  policies.CopyFrom(*device_settings);
  std::unique_ptr<em::ChromeDeviceSettingsProto> off_device_settings =
      policy::DeviceOffHoursController::ApplyOffHoursMode(&policies);

  bool off_hours_mode = false;
  if (off_device_settings) {
    off_hours_mode = true;
  }

  dbus::MessageWriter writer(response.get());
  writer.AppendBool(off_hours_mode);

  response_sender.Run(std::move(response));
}

}  // namespace chromeos