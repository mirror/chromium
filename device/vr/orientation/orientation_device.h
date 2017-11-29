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

class OrientationDevice : public VRDeviceBase, public mojom::SensorClient {
 public:
  OrientationDevice(device::mojom::SensorProviderPtr& sensor_provider);
  ~OrientationDevice() override;

  // VRDeviceBase
  void OnMagicWindowPoseRequest(
      mojom::VRMagicWindowProvider::GetPoseCallback callback) override;

  void SetReadyCallback(base::OnceClosure callback) {
    ready_callback_ = std::move(callback);
  };

  bool IsFallbackDevice() override;

  // Indicates whether the device was able to connect to orientation events.
  bool IsAvailable() const { return available_; };

 private:
  // SensorClient Functions.
  void RaiseError() override;
  void SensorReadingChanged() override{};

  // Sensor event reaction functions.
  void SensorReady(device::mojom::SensorInitParamsPtr params);
  void HandleSensorError();
  void OnSensorAddConfiguration(bool success);

  gfx::Quaternion SensorSpaceToWorldSpace(gfx::Quaternion q);
  gfx::Quaternion WorldSpaceToUserOrientedSpace(gfx::Quaternion q);

  // These functions should do nothing as they should never be called.
  void RequestPresent(
      VRDisplayImpl* display,
      mojom::VRSubmitFrameClientPtr submit_client,
      mojom::VRPresentationProviderRequest request,
      mojom::VRDisplayHost::RequestPresentCallback callback) override;
  void ExitPresent() override;

  bool available_ = false;
  base::OnceClosure ready_callback_;

  // The initial state of the world used to define forwards.
  base::Optional<gfx::Quaternion> base_pose_;
  gfx::Quaternion latest_pose_;

  device::mojom::SensorPtr sensor_;
  device::PlatformSensorConfiguration default_config_;
  mojo::ScopedSharedBufferHandle shared_buffer_handle_;
  mojo::ScopedSharedBufferMapping shared_buffer_;
  std::unique_ptr<device::SensorReadingSharedBufferReader>
      shared_buffer_reader_;
  mojo::Binding<mojom::SensorClient> binding_;
};

}  // namespace device

#endif  // DEVICE_VR_ORIENTATION_DEVICE_H
