// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/fake_device_sync.h"

namespace multidevice {

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
  std::move(callback).Run(synced_devices_);
}

void FakeDeviceSync::SetCapabilityEnabled(
    const std::string& device_id,
    cryptauth::DeviceCapabilityManager::Capability capability,
    bool enabled,
    SetCapabilityEnabledCallback callback) {
  std::move(callback).Run(device_sync::mojom::SetCapabilityResponse::New(
      capability_enabled_result_code_));
};

void FakeDeviceSync::FindEligibleDevicesForCapability(
    cryptauth::DeviceCapabilityManager::Capability capability,
    FindEligibleDevicesForCapabilityCallback callback) {
  std::move(callback).Run(device_sync::mojom::FindEligibleDevicesResponse::New(
      find_eligible_devices_for_capability_result_code_,
      device_ids_for_eligible_devices_, ineligible_devices_));
};

void FakeDeviceSync::IsCapabilityPromotable(
    const std::string& device_id,
    cryptauth::DeviceCapabilityManager::Capability capability,
    IsCapabilityPromotableCallback callback) {
  std::move(callback).Run(
      device_sync::mojom::IsCapabilityPromotableResponse::New(
          is_capability_promotable_result_code_, is_capability_promotable_));
};

void FakeDeviceSync::GetUserPublicKey(GetUserPublicKeyCallback callback) {
  std::move(callback).Run(public_key_);
};

}  // namespace multidevice