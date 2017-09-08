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
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace multidevice {

// Test double for DeviceSync.  Currently used to only test the service rather
// than implementation.
class FakeDeviceSync : public device_sync::mojom::DeviceSync {
 public:
  class Factory {
   public:
    static std::unique_ptr<FakeDeviceSync> NewInstance(
        std::unique_ptr<service_manager::ServiceContextRef> service_ref);

    static void SetInstanceForTesting(Factory* factory);

   protected:
    virtual std::unique_ptr<FakeDeviceSync> BuildInstance(
        std::unique_ptr<service_manager::ServiceContextRef> service_ref);

   private:
    static Factory* factory_instance_;
  };

  explicit FakeDeviceSync(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);

  ~FakeDeviceSync() override;

  // mojom::DeviceSync:
  void ForceEnrollmentNow() override;
  void ForceSyncNow() override;
  void AddObserver(device_sync::mojom::DeviceSyncObserverPtr observer) override;

 private:
  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  mojo::InterfacePtrSet<device_sync::mojom::DeviceSyncObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(FakeDeviceSync);
};

}  // namespace multidevice

#endif  // COMPONENTS_MULTIDEVICE_FAKE_DEVICE_SYNC_H_
