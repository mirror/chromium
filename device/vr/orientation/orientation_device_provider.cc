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

OrientationDeviceProvider::~OrientationDeviceProvider() = default;

void OrientationDeviceProvider::Initialize(
    base::Callback<void(VRDevice*)> add_device_callback,
    base::Callback<void(VRDevice*)> remove_device_callback,
    base::OnceClosure initialization_complete) {
  if (device_ && device_->IsAvailable()) {
    add_device_callback.Run(device_.get());
    return;
  }

  if (!device_) {
    device_ = std::make_unique<OrientationDevice>(sensor_provider_);
    add_device_callback_ = add_device_callback;
    initialized_callback_ = std::move(initialization_complete);
    device_->SetReadyCallback(base::BindOnce(
        &OrientationDeviceProvider::DeviceInitialized, base::Unretained(this)));
  }
}

bool OrientationDeviceProvider::Initialized() {
  return initialized_;
};

void OrientationDeviceProvider::DeviceInitialized() {
  // This should only be called after the device is initialized.
  DCHECK(device_);
  // This should only be called once.
  DCHECK(!initialized_);

  // If the device successfully connected to the orientation APIs, provide it.
  if (device_->IsAvailable()) {
    add_device_callback_.Run(device_.get());
  }

  initialized_ = true;
  std::move(initialized_callback_).Run();
}

}  // namespace device
