// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_
#define COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/cryptauth/device_capability_manager.h"
#include "components/cryptauth/remote_device_provider.h"
#include "components/multidevice/service/public/interfaces/device_sync.mojom.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/service.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace multidevice {

// This class syncs metadata about other devices tied to a given Google account.
// It contacts the existing CryptAuth back-end to enroll the current device
// and sync down new data about other devices.
class DeviceSyncImpl : public device_sync::mojom::DeviceSync {
 public:
  DeviceSyncImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);

  ~DeviceSyncImpl() override;

  // mojom::DeviceSync:
  void ForceEnrollmentNow() override;

  void ForceSyncNow() override;

  void AddObserver(device_sync::mojom::DeviceSyncObserverPtr observer) override;

 private:
  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  mojo::InterfacePtrSet<device_sync::mojom::DeviceSyncObserver> observers_;
  std::unique_ptr<cryptauth::RemoteDeviceProvider> remote_device_provider_;
};  // namespace multidevice

}  // namespace multidevice

#endif  // COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_
