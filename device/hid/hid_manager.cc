// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/hid/hid_manager.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "device/base/device_client.h"
#include "device/hid/hid_connection_manager.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace device {

HidManager::HidManager()
    // TODO(ke.he@intel.com): Temporarily we still keep the HidService being
    // owned and hosted in ChromeDeviceClient. The device service is shutdown
    // earlier than ChromeDeviceClient, it is safe to hold a raw pointer of
    // HidService here. After //device/u2f be mojofied or be moved into device
    // service, we will remove HidService from the ChromeDeviceClient and let
    // HidManager to own the HidService.
    : hid_service_(DeviceClient::Get()->GetHidService()),
      hid_service_observer_(this),
      weak_factory_(this) {
  DCHECK(hid_service_);
  hid_service_observer_.Add(hid_service_);
}

HidManager::~HidManager() {}

void HidManager::AddBinding(device::mojom::HidManagerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void HidManager::GetDevicesAndRegister(
    device::mojom::HidObserverAssociatedPtrInfo observer_info,
    device::mojom::HidManager::GetDevicesCallback callback) {
  hid_service_->GetDevices(
      base::Bind(&HidManager::CreateDeviceList, weak_factory_.GetWeakPtr(),
                 base::Passed(&callback), base::Passed(&observer_info)));
}

void HidManager::GetDevices(
    device::mojom::HidManager::GetDevicesCallback callback) {
  // Create a invalid HidObserverAssociatedPtrInfo.
  device::mojom::HidObserverAssociatedPtrInfo observer_info;

  hid_service_->GetDevices(
      base::Bind(&HidManager::CreateDeviceList, weak_factory_.GetWeakPtr(),
                 base::Passed(&callback), base::Passed(&observer_info)));
}

void HidManager::CreateDeviceList(
    device::mojom::HidManager::GetDevicesCallback callback,
    device::mojom::HidObserverAssociatedPtrInfo observer_info,
    std::vector<device::mojom::HidDeviceInfoPtr> devices) {
  std::vector<device::mojom::HidDeviceInfoPtr> device_list;
  for (auto& device : devices)
    device_list.push_back(device->Clone());

  std::move(callback).Run(std::move(device_list));

  if (!observer_info.is_valid())
    return;

  device::mojom::HidObserverAssociatedPtr observer_ptr;
  observer_ptr.Bind(std::move(observer_info));
  observers_.AddPtr(std::move(observer_ptr));
}

void HidManager::Connect(const std::string& device_guid,
                         device::mojom::HidManager::ConnectCallback callback) {
  hid_service_->Connect(device_guid, base::Bind(&HidManager::CreateConnection,
                                                weak_factory_.GetWeakPtr(),
                                                base::Passed(&callback)));
}

void HidManager::CreateConnection(
    device::mojom::HidManager::ConnectCallback callback,
    scoped_refptr<device::HidConnection> connection) {
  if (!connection) {
    std::move(callback).Run(nullptr);
    return;
  }

  device::mojom::HidConnectionPtr client;
  auto binding = mojo::MakeStrongBinding(
      base::MakeUnique<HidConnectionManager>(connection),
      mojo::MakeRequest(&client));
  std::move(callback).Run(std::move(client));
}

void HidManager::OnDeviceAdded(device::mojom::HidDeviceInfoPtr device) {
  device::mojom::HidDeviceInfo* device_info = device.get();
  observers_.ForAllPtrs([device_info](device::mojom::HidObserver* observer) {
    observer->DeviceAdded(device_info->Clone());
  });
}

void HidManager::OnDeviceRemoved(device::mojom::HidDeviceInfoPtr device) {
  device::mojom::HidDeviceInfo* device_info = device.get();
  observers_.ForAllPtrs([device_info](device::mojom::HidObserver* observer) {
    observer->DeviceRemoved(device_info->Clone());
  });
}

}  // namespace device
