// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_
#define COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_

namespace multidevice {

class DeviceSyncImpl : public mojom::DeviceSync {
 public:
  DeviceSyncImpl();

  ~DeviceSync() override;

  void ForceEnrollmentNow() override;

  // void ForceSyncNow() override;

  // bool GetSyncedDevices(std::vector<RemoteDevicePtr>* devices) override;

  // void GetSyncedDevices(GetSyncedDevicesCallback callback) override;

  // void SetCapabilityEnabled(const std::string& device_id,
  // RemoteDeviceCapability capability, bool enabled,
  // SetCapabilityEnabledCallback callback) override;

  // void FindEligibleDevicesForCapability(RemoteDeviceCapability capability,
  // FindEligibleDevicesForCapabilityCallback callback) override;

  // void IsCapabilityPromotable(const std::string& device_id,
  // RemoteDeviceCapability capability, IsCapabilityPromotableCallback callback)
  // override;

  // Sync method. This signature is used by the client side; the service side
  // should implement the signature with callback below.
  // bool GetUserPublicKey(std::string* public_key) override;

  // void GetUserPublicKey(GetUserPublicKeyCallback callback) override;

  void AddObserver(DeviceSyncObserverPtr observer) override;

 private:
  mojo::InterfacePtrSet<mojom::DeviceSync> listerners_;
};

}  // namespace multidevice

#endif COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_
