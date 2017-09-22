// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/hid/hid_manager_impl.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/lazy_instance.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "device/base/device_client.h"
#include "device/hid/hid_connection_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace device {

base::LazyInstance<device::HidService*>::Leaky g_hid_service =
    LAZY_INSTANCE_INITIALIZER;

HidManagerImpl::HidManagerImpl()
    : hid_service_observer_(this), weak_factory_(this) {
  if (!g_hid_service.Get())
    // |g_hid_service| takes the ownership of the HidService instance.
    g_hid_service.Get() = HidService::Create();

  DCHECK(g_hid_service.Get());
  hid_service_observer_.Add(g_hid_service.Get());
}

HidManagerImpl::~HidManagerImpl() {}

// static
void HidManagerImpl::SetHidServiceForTesting(device::HidService* hid_service) {
  g_hid_service.Get() = hid_service;
}

void HidManagerImpl::AddBinding(device::mojom::HidManagerRequest request) {
  bindings_.AddBinding(this, std::move(request));
}

void HidManagerImpl::GetDevicesAndSetClient(
    device::mojom::HidManagerClientAssociatedPtrInfo client,
    GetDevicesCallback callback) {
  g_hid_service.Get()->GetDevices(AdaptCallbackForRepeating(base::BindOnce(
      &HidManagerImpl::CreateDeviceList, weak_factory_.GetWeakPtr(),
      std::move(callback), std::move(client))));
}

void HidManagerImpl::GetDevices(GetDevicesCallback callback) {
  g_hid_service.Get()->GetDevices(AdaptCallbackForRepeating(base::BindOnce(
      &HidManagerImpl::CreateDeviceList, weak_factory_.GetWeakPtr(),
      std::move(callback), nullptr)));
}

void HidManagerImpl::CreateDeviceList(
    GetDevicesCallback callback,
    device::mojom::HidManagerClientAssociatedPtrInfo client,
    std::vector<device::mojom::HidDeviceInfoPtr> devices) {
  std::move(callback).Run(std::move(devices));

  if (!client.is_valid())
    return;

  device::mojom::HidManagerClientAssociatedPtr client_ptr;
  client_ptr.Bind(std::move(client));
  clients_.AddPtr(std::move(client_ptr));
}

void HidManagerImpl::Connect(const std::string& device_guid,
                             ConnectCallback callback) {
  g_hid_service.Get()->Connect(
      device_guid, AdaptCallbackForRepeating(base::BindOnce(
                       &HidManagerImpl::CreateConnection,
                       weak_factory_.GetWeakPtr(), std::move(callback))));
}

void HidManagerImpl::CreateConnection(
    ConnectCallback callback,
    scoped_refptr<device::HidConnection> connection) {
  if (!connection) {
    std::move(callback).Run(nullptr);
    return;
  }

  device::mojom::HidConnectionPtr client;
  mojo::MakeStrongBinding(base::MakeUnique<HidConnectionImpl>(connection),
                          mojo::MakeRequest(&client));
  std::move(callback).Run(std::move(client));
}

void HidManagerImpl::OnDeviceAdded(device::mojom::HidDeviceInfoPtr device) {
  device::mojom::HidDeviceInfo* device_info = device.get();
  clients_.ForAllPtrs([device_info](device::mojom::HidManagerClient* client) {
    client->DeviceAdded(device_info->Clone());
  });
}

void HidManagerImpl::OnDeviceRemoved(device::mojom::HidDeviceInfoPtr device) {
  device::mojom::HidDeviceInfo* device_info = device.get();
  clients_.ForAllPtrs([device_info](device::mojom::HidManagerClient* client) {
    client->DeviceRemoved(device_info->Clone());
  });
}

}  // namespace device
