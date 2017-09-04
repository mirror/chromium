// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_HID_HID_MANAGER_H_
#define DEVICE_HID_HID_MANAGER_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "device/hid/hid_device_info.h"
#include "device/hid/hid_service.h"
#include "device/hid/public/interfaces/hid.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"

namespace device {

// HidManager is owned by Device Service. It is reponsible for handling mojo
// communications from clients. It asks the HidService to delegate the real work
// of talking with different platforms.
class HidManager : public device::mojom::HidManager,
                   public device::HidService::Observer {
 public:
  HidManager();
  ~HidManager() override;

  void AddBinding(device::mojom::HidManagerRequest request);

  // device::mojom::HidManager implementation:
  void GetDevicesAndRegister(
      device::mojom::HidObserverAssociatedPtrInfo observer_info,
      device::mojom::HidManager::GetDevicesCallback callback) override;

  void GetDevices(
      device::mojom::HidManager::GetDevicesCallback callback) override;

  void Connect(const std::string& device_guid,
               device::mojom::HidManager::ConnectCallback callback) override;

 private:
  void CreateDeviceList(device::mojom::HidManager::GetDevicesCallback callback,
                        device::mojom::HidObserverAssociatedPtrInfo observer,
                        std::vector<device::mojom::HidDeviceInfoPtr> devices);

  void CreateConnection(device::mojom::HidManager::ConnectCallback callback,
                        scoped_refptr<HidConnection> connection);

  // HidService::Observer:
  void OnDeviceAdded(device::mojom::HidDeviceInfoPtr device_info) override;
  void OnDeviceRemoved(device::mojom::HidDeviceInfoPtr device_info) override;

  HidService* hid_service_;
  mojo::BindingSet<device::mojom::HidManager> bindings_;
  mojo::AssociatedInterfacePtrSet<device::mojom::HidObserver> observers_;
  ScopedObserver<device::HidService, device::HidService::Observer>
      hid_service_observer_;

  base::WeakPtrFactory<HidManager> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(HidManager);
};

}  // namespace device

#endif  // DEVICE_HID_HID_MANAGER_H_
