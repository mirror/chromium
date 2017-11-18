// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/fake_gatt_services_workaround.h"

namespace chromeos {

namespace tether {

FakeGattServicesWorkaround::FakeGattServicesWorkaround() {}

FakeGattServicesWorkaround::~FakeGattServicesWorkaround() {}

void FakeGattServicesWorkaround::NotifyAsynchronousShutdownComplete() {
  GattServicesWorkaround::NotifyAsynchronousShutdownComplete();
}

void FakeGattServicesWorkaround::RequestGattServicesForDevice(
    const cryptauth::RemoteDevice& remote_device) {
  requested_device_ids_.push_back(remote_device.GetDeviceId());
}

bool FakeGattServicesWorkaround::HasPendingRequests() {
  return has_pending_requests_;
}

}  // namespace tether

}  // namespace chromeos
