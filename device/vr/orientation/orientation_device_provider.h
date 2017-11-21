// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_ORIENTATION_DEVICE_PROVIDER_H
#define DEVICE_VR_ORIENTATION_DEVICE_PROVIDER_H

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "device/vr/vr_device_provider.h"
#include "device/vr/vr_export.h"
#include "services/device/public/interfaces/constants.mojom.h"
#include "services/device/public/interfaces/sensor_provider.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace device {

class DEVICE_VR_EXPORT OrientationDeviceProvider : public VRDeviceProvider {
 public:
  OrientationDeviceProvider(service_manager::Connector* connector);
  ~OrientationDeviceProvider() override;

  void GetDevices(std::vector<VRDevice*>* devices) override;
  void Initialize() override;

 private:
  bool initialized_;
  device::mojom::SensorProviderPtr sensor_provider_;

  DISALLOW_COPY_AND_ASSIGN(OrientationDeviceProvider);
};

}  // namespace device

#endif  // DEVICE_VR_ORIENTATION_DEVICE_PROVIDER_H
