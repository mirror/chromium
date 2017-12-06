// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_VR_ORIENTATION_DEVICE_H
#define DEVICE_VR_ORIENTATION_DEVICE_H

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/threading/simple_thread.h"
#include "device/vr/vr_device_base.h"
#include "device/vr/vr_service.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/device/public/cpp/generic_sensor/sensor_reading.h"
#include "services/device/public/cpp/generic_sensor/sensor_reading_shared_buffer_reader.h"
#include "services/device/public/interfaces/sensor_provider.mojom.h"
#include "ui/gfx/geometry/quaternion.h"

namespace device {

class DEVICE_VR_EXPORT VROrientationDevice : public VRDeviceBase,
                                             public mojom::SensorClient {
 public:
  VROrientationDevice(mojom::SensorProviderPtr& sensor_provider,
                      base::OnceClosure ready_callback);
  ~VROrientationDevice() override;

  // VRDeviceBase
  void OnMagicWindowPoseRequest(
      mojom::VRMagicWindowProvider::GetPoseCallback callback) override;

  bool IsFallbackDevice() override;

  // Indicates whether the device was able to connect to orientation events.
  bool IsAvailable() const { return available_; }

 private:
  // SensorClient Functions.
  void RaiseError() override;
  void SensorReadingChanged() override {}

  // Sensor event reaction functions.
  void SensorReady(device::mojom::SensorInitParamsPtr params);
  void HandleSensorError();
  void OnSensorAddConfiguration(bool success);

  gfx::Quaternion SensorSpaceToWorldSpace(gfx::Quaternion q);
  gfx::Quaternion WorldSpaceToUserOrientedSpace(gfx::Quaternion q);

  bool available_ = false;
  base::OnceClosure ready_callback_;

  // The initial state of the world used to define forwards.
  base::Optional<gfx::Quaternion> base_pose_;
  gfx::Quaternion latest_pose_;

  device::mojom::SensorPtr sensor_;
  mojo::ScopedSharedBufferHandle shared_buffer_handle_;
  mojo::ScopedSharedBufferMapping shared_buffer_;
  std::unique_ptr<device::SensorReadingSharedBufferReader>
      shared_buffer_reader_;
  mojo::Binding<mojom::SensorClient> binding_;
};

}  // namespace device

#endif  // DEVICE_VR_ORIENTATION_DEVICE_H
