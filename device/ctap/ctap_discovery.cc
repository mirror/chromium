// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/ctap/ctap_discovery.h"

#include <utility>

namespace device {

CTAPDiscovery::CTAPDiscovery() = default;

CTAPDiscovery::~CTAPDiscovery() = default;

void CTAPDiscovery::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void CTAPDiscovery::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void CTAPDiscovery::NotifyDiscoveryStarted(bool success) {
  for (auto& observer : observers_)
    observer.DiscoveryStarted(this, success);
}

void CTAPDiscovery::NotifyDiscoveryStopped(bool success) {
  for (auto& observer : observers_)
    observer.DiscoveryStopped(this, success);
}

void CTAPDiscovery::NotifyDeviceAdded(FidoDevice* device) {
  for (auto& observer : observers_)
    observer.DeviceAdded(this, device);
}

void CTAPDiscovery::NotifyDeviceRemoved(FidoDevice* device) {
  for (auto& observer : observers_)
    observer.DeviceRemoved(this, device);
}

std::vector<FidoDevice*> CTAPDiscovery::GetDevices() {
  std::vector<FidoDevice*> devices;
  devices.reserve(devices_.size());
  for (const auto& device : devices_)
    devices.push_back(device.second.get());
  return devices;
}

std::vector<const FidoDevice*> CTAPDiscovery::GetDevices() const {
  std::vector<const FidoDevice*> devices;
  devices.reserve(devices_.size());
  for (const auto& device : devices_)
    devices.push_back(device.second.get());
  return devices;
}

FidoDevice* CTAPDiscovery::GetDevice(base::StringPiece device_id) {
  return const_cast<FidoDevice*>(
      static_cast<const CTAPDiscovery*>(this)->GetDevice(device_id));
}

const FidoDevice* CTAPDiscovery::GetDevice(base::StringPiece device_id) const {
  auto found = devices_.find(device_id);
  return found != devices_.end() ? found->second.get() : nullptr;
}

bool CTAPDiscovery::AddDevice(std::unique_ptr<FidoDevice> device) {
  std::string device_id = device->GetId();
  const auto result = devices_.emplace(std::move(device_id), std::move(device));
  if (result.second)
    NotifyDeviceAdded(result.first->second.get());
  return result.second;
}

bool CTAPDiscovery::RemoveDevice(base::StringPiece device_id) {
  auto found = devices_.find(device_id);
  if (found == devices_.end())
    return false;

  auto device = std::move(found->second);
  devices_.erase(found);
  NotifyDeviceRemoved(device.get());
  return true;
}

}  // namespace device
