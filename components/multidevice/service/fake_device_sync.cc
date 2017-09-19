// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/fake_device_sync.h"

namespace multidevice {

std::vector<cryptauth::RemoteDevice> FakeDeviceSync::remote_devices_ = {
    cryptauth::RemoteDevice(), cryptauth::RemoteDevice(),
    cryptauth::RemoteDevice()};

FakeDeviceSync::FakeDeviceSync() {}

FakeDeviceSync::~FakeDeviceSync() {}

void FakeDeviceSync::ForceEnrollmentNow() {
  observers_.ForAllPtrs(
      [this](device_sync::mojom::DeviceSyncObserver* observer) {
        observer->OnEnrollmentFinished(should_enroll_successfully_);
      });
}

void FakeDeviceSync::ForceSyncNow() {
  observers_.ForAllPtrs([this](
                            device_sync::mojom::DeviceSyncObserver* observer) {
    observer->OnDevicesSynced(should_sync_successfully_, device_change_result_);
  });
}

void FakeDeviceSync::AddObserver(
    device_sync::mojom::DeviceSyncObserverPtr observer) {
  observers_.AddPtr(std::move(observer));
}

void FakeDeviceSync::GetSyncedDevices(GetSyncedDevicesCallback callback) {
  std::move(callback).Run(remote_devices_);
}

}  // namespace multidevice