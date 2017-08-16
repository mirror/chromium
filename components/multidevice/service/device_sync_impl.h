// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_
#define COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/multidevice/service/public/interfaces/device_sync.mojom.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace multidevice {

class DeviceSyncImpl : public device_sync::mojom::DeviceSync {
 public:
  DeviceSyncImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);

  ~DeviceSyncImpl() override;

  void ForceEnrollmentNow() override;

  void ForceSyncNow() override;

  // mojom::DeviceSync:
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

  void AddObserver(device_sync::mojom::DeviceSyncObserverPtr observer) override;

 private:
  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  mojo::InterfacePtrSet<device_sync::mojom::DeviceSync> listeners_;
};

}  // namespace multidevice

#endif  // COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_
