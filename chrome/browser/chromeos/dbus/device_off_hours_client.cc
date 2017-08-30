// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/dbus/device_off_hours_client.h"

#include "base/bind.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace chromeos {

DeviceOffHoursClient::DeviceOffHoursClient() : weak_ptr_factory_(this) {}

DeviceOffHoursClient::~DeviceOffHoursClient() {}

void DeviceOffHoursClient::Start(
    scoped_refptr<dbus::ExportedObject> exported_object) {
  LOG(ERROR) << "Daria start off hours client";
  exported_object->ExportMethod(system_clock::kSystemClockInterface,
                                system_clock::kSystemLastSyncInfo,
                                base::Bind(&DeviceOffHoursClient::LastSyncInfo,
                                           weak_ptr_factory_.GetWeakPtr()),
                                base::Bind(&DeviceOffHoursClient::OnExported,
                                           weak_ptr_factory_.GetWeakPtr()));
}

void DeviceOffHoursClient::OnExported(const std::string& interface_name,
                                      const std::string& method_name,
                                      bool success) {
  LOG(ERROR) << "Daria OnExported " << success;
  if (!success) {
    LOG(ERROR) << "Failed to export " << interface_name << "." << method_name;
  }
}

void DeviceOffHoursClient::LastSyncInfo(
    dbus::MethodCall* method_call,
    dbus::ExportedObject::ResponseSender response_sender) {
  LOG(ERROR) << "Daria LastSyncInfo ";
  if (!method_call)
    return;
  std::unique_ptr<dbus::Response> response =
      dbus::Response::FromMethodCall(method_call);
  LOG(ERROR) << "Daria WE GOT MESSAGE " << response->ToString();

  response_sender.Run(std::move(response));
}

}  // namespace chromeos
