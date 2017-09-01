// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/dbus/device_off_hours_client.h"

#include "base/bind.h"
#include "base/message_loop/message_loop_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

//#include "chromeos/chromeos_export.h"

namespace chromeos {

DeviceOffHoursClient::DeviceOffHoursClient() : weak_ptr_factory_(this) {}

DeviceOffHoursClient::~DeviceOffHoursClient() {}

// static
DeviceOffHoursClient* DeviceOffHoursClient::Create() {
  return new DeviceOffHoursClient();
}

void DeviceOffHoursClient::Init(dbus::Bus* bus) {
  system_clock_proxy_ = bus->GetObjectProxy(
      system_clock::kSystemClockServiceName,
      dbus::ObjectPath(system_clock::kSystemClockServicePath));
  system_clock_proxy_->WaitForServiceToBeAvailable(
      base::Bind(&DeviceOffHoursClient::ServiceInitiallyAvailable,
                 weak_ptr_factory_.GetWeakPtr()));
}

void DeviceOffHoursClient::ServiceInitiallyAvailable(
    bool service_is_available) {
  LOG(ERROR) << "Daria Service available " << service_is_available;
  if (service_is_available)
    GetSystemClockLastSyncInfo();
  else
    LOG(ERROR) << "Failed to wait for D-Bus service availability";
}

bool DeviceOffHoursClient::IsNetworkSyncronized() {
  system_clock_proxy_->WaitForServiceToBeAvailable(
      base::Bind(&DeviceOffHoursClient::ServiceInitiallyAvailable,
                 weak_ptr_factory_.GetWeakPtr()));
  return network_synchronized_;
}

void DeviceOffHoursClient::GetSystemClockLastSyncInfo() {
  dbus::MethodCall method_call(system_clock::kSystemClockInterface,
                               system_clock::kSystemLastSyncInfo);
  system_clock_proxy_->CallMethod(
      &method_call, dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
      base::Bind(&DeviceOffHoursClient::OnGotSystemClockLastSyncInfo,
                 weak_ptr_factory_.GetWeakPtr()));
}

void DeviceOffHoursClient::OnGotSystemClockLastSyncInfo(
    dbus::Response* response) {
  const base::TimeDelta kMillisecondsInMinutes =
      base::TimeDelta::FromMinutes(1);
  if (!response) {
    network_synchronized_ = false;
    LOG(ERROR) << system_clock::kSystemClockInterface << "."
               << system_clock::kSystemLastSyncInfo << " request failed.";
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&DeviceOffHoursClient::GetSystemClockLastSyncInfo,
                   weak_ptr_factory_.GetWeakPtr()),
        kMillisecondsInMinutes);
    return;
  }

  dbus::MessageReader reader(response);
  bool network_synchronized = false;
  if (!reader.PopBool(&network_synchronized)) {
    network_synchronized_ = false;
    LOG(ERROR) << system_clock::kSystemClockInterface << "."
               << system_clock::kSystemLastSyncInfo
               << " response lacks network-synchronized argument";
    return;
  }
  LOG(ERROR) << "network sync = " << network_synchronized;
  if (network_synchronized) {
    network_synchronized_ = true;
  } else {
    network_synchronized_ = false;
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE,
        base::Bind(&DeviceOffHoursClient::GetSystemClockLastSyncInfo,
                   weak_ptr_factory_.GetWeakPtr()),
        kMillisecondsInMinutes);
  }
}

}  // namespace chromeos