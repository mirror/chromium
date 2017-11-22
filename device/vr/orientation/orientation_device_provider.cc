// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/orientation/orientation_device_provider.h"
#include "base/callback.h"
#include "device/vr/orientation/orientation_device.h"
#include "services/device/public/interfaces/sensor_provider.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/identity.h"

namespace device {

using GetDeviceCallback = base::OnceCallback<void(VRDevice*)>;

OrientationDeviceProvider::OrientationDeviceProvider(
    service_manager::Connector* connector) {
  connector->BindInterface(device::mojom::kServiceName,
                           mojo::MakeRequest(&sensor_provider_));
  sensor_provider_.set_connection_error_handler(
      base::BindOnce(&device::mojom::SensorProviderPtr::reset,
                     base::Unretained(&sensor_provider_)));
}

OrientationDeviceProvider::~OrientationDeviceProvider() {}

void OrientationDeviceProvider::Initialize() {
  initialized_ = true;
  device_ = std::make_unique<OrientationDevice>(sensor_provider_);
  device_->SetReadyCallback(base::BindOnce(
      &OrientationDeviceProvider::DeviceReady, base::Unretained(this)));
}

void OrientationDeviceProvider::GetDevices(std::vector<VRDevice*>* devices) {
  if (initialized_ && device_->IsAvailable()) {
    devices->push_back(device_.get());
  }
}

void OrientationDeviceProvider::GetDevice(GetDeviceCallback callback) {
  if (device_ready_) {
    if (device_->IsAvailable()) {
      std::move(callback).Run(device_.get());
    } else {
      std::move(callback).Run(nullptr);
    }
  } else {
    pending_callbacks_.emplace_back(std::move(callback));
  }
}

void OrientationDeviceProvider::DeviceReady() {
  device_ready_ = true;
  for (auto it = pending_callbacks_.begin(); it != pending_callbacks_.end();
       it++) {
    if (device_->IsAvailable()) {
      std::move(*it).Run(device_.get());
    } else {
      std::move(*it).Run(nullptr);
    }
  }

  pending_callbacks_.clear();
}

}  // namespace device
