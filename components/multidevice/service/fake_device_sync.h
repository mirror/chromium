// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_FAKE_DEVICE_SYNC_H_
#define COMPONENTS_MULTIDEVICE_FAKE_DEVICE_SYNC_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/multidevice/service/public/interfaces/device_sync.mojom.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace multidevice {

// Test double for DeviceSync.
class FakeDeviceSync : public device_sync::mojom::DeviceSync {
 public:
  FakeDeviceSync();

  ~FakeDeviceSync() override;

  bool should_sync_successfully() { return should_sync_successfully_; }

  bool should_enroll_successfully() { return should_enroll_successfully_; }

  void set_should_sync_successfully(bool should_sync_successfully) {
    should_sync_successfully_ = should_sync_successfully;
  }

  void set_should_enroll_successfully(bool should_enroll_successfully) {
    should_enroll_successfully_ = should_enroll_successfully;
  }

  void set_device_change_result(bool device_change_result) {
    device_change_result_ = device_change_result;
  }

  // mojom::DeviceSync:
  void ForceEnrollmentNow() override;
  void ForceSyncNow() override;
  void AddObserver(device_sync::mojom::DeviceSyncObserverPtr observer) override;
  void GetSyncedDevices(GetSyncedDevicesCallback callback) override;
  void SetCapabilityEnabled(
      const std::string& device_id,
      cryptauth::DeviceCapabilityManager::Capability capability,
      bool enabled,
      SetCapabilityEnabledCallback callback) override;
  void FindEligibleDevicesForCapability(
      cryptauth::DeviceCapabilityManager::Capability capability,
      FindEligibleDevicesForCapabilityCallback callback) override;
  void IsCapabilityPromotable(
      const std::string& device_id,
      cryptauth::DeviceCapabilityManager::Capability capability,
      IsCapabilityPromotableCallback callback) override;
  void GetUserPublicKey(GetUserPublicKeyCallback callback) override;

 private:
  bool device_change_result_ = true;
  bool should_sync_successfully_ = true;
  bool should_enroll_successfully_ = true;
  mojo::InterfacePtrSet<device_sync::mojom::DeviceSyncObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(FakeDeviceSync);
};

}  // namespace multidevice

#endif  // COMPONENTS_MULTIDEVICE_FAKE_DEVICE_SYNC_H_
