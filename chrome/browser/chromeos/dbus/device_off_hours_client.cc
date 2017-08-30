// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/dbus/device_off_hours_client.h"

#include <base/message_loop/message_loop.h>
#include <base/rand_util.h>
#include <base/run_loop.h>
#include <dbus/message.h>
#include <stdint.h>

#include "base/task_runner.h"

namespace chromeos {

DeviceOffHoursClient::DeviceOffHoursClient() : weak_ptr_factory_(this) {}

DeviceOffHoursClient::~DeviceOffHoursClient() {}

void DeviceOffHoursClient::Init(dbus::Bus* bus) {
  system_clock_proxy_ = bus->GetObjectProxy(
      system_clock::kSystemClockServiceName,
      dbus::ObjectPath(system_clock::kSystemClockServicePath));
  system_clock_proxy_->ConnectToSignal(
      system_clock::kSystemClockInterface, system_clock::kSystemClockUpdated,
      base::Bind(&DeviceOffHoursClient::TimeUpdatedReceived,
                 weak_ptr_factory_.GetWeakPtr()),
      base::Bind(&DeviceOffHoursClient::TimeUpdatedConnected,
                 weak_ptr_factory_.GetWeakPtr()));
  system_clock_proxy_->WaitForServiceToBeAvailable(
      base::Bind(&DeviceOffHoursClient::ServiceInitiallyAvailable,
                 weak_ptr_factory_.GetWeakPtr()));
}

const base::TimeDelta kMillisecondsInMinutes = base::TimeDelta::FromMinutes(1);

void DeviceOffHoursClient::ServiceInitiallyAvailable(
    bool service_is_available) {
  if (service_is_available)
    GetSystemClockLastSyncInfo();
  else
    LOG(ERROR) << "Failed to wait for D-Bus service availability";
}

// Called when a TimeUpdated signal is received.
void DeviceOffHoursClient::TimeUpdatedReceived(dbus::Signal* signal) {
  VLOG(1) << "TimeUpdated signal received: " << signal->ToString();
  dbus::MessageReader reader(signal);
}

// Called when the TimeUpdated signal is initially connected.
void DeviceOffHoursClient::TimeUpdatedConnected(
    const std::string& interface_name,
    const std::string& signal_name,
    bool success) {
  LOG_IF(ERROR, !success) << "Failed to connect to TimeUpdated signal.";
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
  if (!response) {
    LOG(ERROR) << system_clock::kSystemClockInterface << "."
               << system_clock::kSystemLastSyncInfo << " request failed.";
    //    base::MessageLoop::current()->PostDelayedTask(
    //        FROM_HERE,
    //        base::Bind(&DeviceOffHoursClient::GetSystemClockLastSyncInfo,
    //                              weak_ptr_factory_.GetWeakPtr()),
    //        kMillisecondsInMinutes);
    return;
  }

  dbus::MessageReader reader(response);
  bool network_synchronized = false;
  if (!reader.PopBool(&network_synchronized)) {
    LOG(ERROR) << system_clock::kSystemClockInterface << "."
               << system_clock::kSystemLastSyncInfo
               << " response lacks network-synchronized argument";
    return;
  }

  if (network_synchronized) {
    LOG(ERROR) << "OKKKKKKKKKKKKKKKKKKKKKKKKKKkk";
  }
  //  else {
  //    base::MessageLoop::current()->PostDelayedTask(
  //        FROM_HERE,
  //        base::Bind(&DeviceOffHoursClient::GetSystemClockLastSyncInfo,
  //                              weak_ptr_factory_.GetWeakPtr()),
  //        kMillisecondsInMinutes);
  //  }
}

}  // namespace chromeos
