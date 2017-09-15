// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/fake_hid_impl_for_testing.h"

namespace device {

bool FakeHidConnectionImpl::mock_connection_error_ = false;

FakeHidConnectionImpl::FakeHidConnectionImpl(
    device::mojom::HidDeviceInfoPtr device)
    : device_(std::move(device)) {}

FakeHidConnectionImpl::~FakeHidConnectionImpl() {}

void FakeHidConnectionImpl::Read(ReadCallback callback) {
  std::vector<uint8_t> buffer = {'F', 'a', 'k', 'e', ' ', 'H', 'i', 'd'};
  std::move(callback).Run(true, 0, buffer);
}

void FakeHidConnectionImpl::Write(uint8_t report_id,
                                  const std::vector<uint8_t>& buffer,
                                  WriteCallback callback) {
  if (mock_connection_error_) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(true);
}

void FakeHidConnectionImpl::GetFeatureReport(
    uint8_t report_id,
    GetFeatureReportCallback callback) {
  NOTREACHED();
}

void FakeHidConnectionImpl::SendFeatureReport(
    uint8_t report_id,
    const std::vector<uint8_t>& buffer,
    SendFeatureReportCallback callback) {
  NOTREACHED();
}

FakeHidManager::FakeHidManager(device::mojom::HidManagerRequest request)
    : binding_(this, std::move(request)) {}

FakeHidManager::~FakeHidManager() {}

void FakeHidManager::GetDevicesAndSetClient(
    device::mojom::HidManagerClientAssociatedPtrInfo client,
    GetDevicesCallback callback) {
  std::vector<device::mojom::HidDeviceInfoPtr> device_list;
  for (auto& map_entry : devices_)
    device_list.push_back(map_entry.second->Clone());

  std::move(callback).Run(std::move(device_list));
  client_ptr_.Bind(std::move(client));
}

void FakeHidManager::GetDevices(GetDevicesCallback callback) {
  std::vector<device::mojom::HidDeviceInfoPtr> device_list;
  for (auto& map_entry : devices_)
    device_list.push_back(map_entry.second->Clone());

  std::move(callback).Run(std::move(device_list));
}

void FakeHidManager::Connect(const std::string& device_guid,
                             ConnectCallback callback) {
  bool is_found = false;
  for (const auto& map_entry : devices_) {
    if (map_entry.first == device_guid) {
      is_found = true;
      break;
    }
  }

  if (!is_found) {
    std::move(callback).Run(nullptr);
    return;
  }

  // Strong binds a instance of FakeHidConnctionImpl.
  device::mojom::HidConnectionPtr client;
  mojo::MakeStrongBinding(
      base::MakeUnique<FakeHidConnectionImpl>(devices_[device_guid]->Clone()),
      mojo::MakeRequest(&client));
  std::move(callback).Run(std::move(client));
}

void FakeHidManager::AddDevice(device::mojom::HidDeviceInfoPtr device) {
  if (client_ptr_)
    client_ptr_->DeviceAdded(device->Clone());

  devices_[device->guid] = std::move(device);
}

void FakeHidManager::RemoveDevice(const std::string device_guid) {
  for (const auto& map_entry : devices_) {
    if (map_entry.first == device_guid) {
      if (client_ptr_)
        client_ptr_->DeviceRemoved(map_entry.second->Clone());

      devices_.erase(map_entry.first);
      return;
    }
  }
}

}  // namespace device
