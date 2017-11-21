// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/orientation/orientation_device_provider.h"
#include "device/vr/orientation/orientation_device.h"
#include "services/device/public/interfaces/sensor_provider.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/identity.h"

namespace device {

OrientationDeviceProvider::OrientationDeviceProvider(
    service_manager::Connector* connector)
    : initialized_(false) {
  connector->BindInterface(device::mojom::kServiceName,
                           mojo::MakeRequest(&sensor_provider_));
  sensor_provider_.set_connection_error_handler(
      base::BindOnce(&device::mojom::SensorProviderPtr::reset,
                     base::Unretained(&sensor_provider_)));
}

OrientationDeviceProvider::~OrientationDeviceProvider() {}

void OrientationDeviceProvider::GetDevices(std::vector<VRDevice*>* devices) {
  if (initialized_) {
    VRDevice* device = new OrientationDevice(sensor_provider_);
    devices->push_back(device);
  }
}

void OrientationDeviceProvider::Initialize() {
  // TODO(offenwanger): Check if orientation events are availible
  initialized_ = true;
}

}  // namespace device
