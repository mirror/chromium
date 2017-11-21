

// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <math.h>

#include "base/memory/ptr_util.h"
#include "base/numerics/math_constants.h"
#include "base/time/time.h"
#include "device/vr/orientation/orientation_device.h"
#include "services/device/public/interfaces/sensor_provider.mojom.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/quaternion.h"
#include "ui/gfx/geometry/vector3d_f.h"

namespace device {

using mojom::SensorType;
using gfx::Quaternion;
using gfx::Vector3dF;

namespace {
static const std::string DEVICE_NAME = "bob";
static constexpr int kDefaultPumpFrequencyHz = 60;

mojom::VREyeParametersPtr CreateEyeParamater() {
  // These will never be used as this device does not present, but are required
  // for compilation.
  mojom::VREyeParametersPtr eye_params = mojom::VREyeParameters::New();
  eye_params->fieldOfView = mojom::VRFieldOfView::New();
  eye_params->offset.resize(3);
  eye_params->renderWidth = 0;
  eye_params->renderHeight = 0;

  eye_params->fieldOfView->upDegrees = 0;
  eye_params->fieldOfView->downDegrees = 0;
  eye_params->fieldOfView->leftDegrees = 0;
  eye_params->fieldOfView->rightDegrees = 0;

  eye_params->offset[0] = 0;
  eye_params->offset[1] = 0;
  eye_params->offset[2] = 0;
  return eye_params;
}

mojom::VRDisplayInfoPtr CreateVRDisplayInfo(unsigned int id) {
  mojom::VRDisplayInfoPtr display_info = mojom::VRDisplayInfo::New();
  display_info->index = id;
  display_info->displayName = DEVICE_NAME;
  display_info->capabilities = mojom::VRDisplayCapabilities::New();
  display_info->capabilities->hasPosition = false;
  display_info->capabilities->hasExternalDisplay = false;
  display_info->capabilities->canPresent = false;

  display_info->leftEye = CreateEyeParamater();
  display_info->rightEye = CreateEyeParamater();

  return display_info;
}

}  // namespace

OrientationDevice::OrientationDevice(
    device::mojom::SensorProviderPtr& sensor_provider) {
  sensor_provider->GetSensor(
      SensorType::ABSOLUTE_ORIENTATION_QUATERNION,
      base::Bind(&OrientationDevice::SensorReady, base::Unretained(this)));

  SetVRDisplayInfo(CreateVRDisplayInfo(GetId()));
}

OrientationDevice::~OrientationDevice() {}

void OrientationDevice::SensorReady(device::mojom::SensorInitParamsPtr params) {
  VLOG(0) << "Got sensor";
  if (!params) {
    VLOG(0) << "No params...";
    HandleSensorError();
    return;
  }

  constexpr size_t kReadBufferSize = sizeof(device::SensorReadingSharedBuffer);

  DCHECK_EQ(0u, params->buffer_offset % kReadBufferSize);

  mode = params->mode;
  default_config = params->default_configuration;

  sensor.Bind(std::move(params->sensor));

  shared_buffer_handle = std::move(params->memory);
  DCHECK(!shared_buffer);
  shared_buffer =
      shared_buffer_handle->MapAtOffset(kReadBufferSize, params->buffer_offset);
  if (!shared_buffer) {
    HandleSensorError();
    return;
  }

  const device::SensorReadingSharedBuffer* buffer =
      static_cast<const device::SensorReadingSharedBuffer*>(
          shared_buffer.get());
  shared_buffer_reader.reset(
      new device::SensorReadingSharedBufferReader(buffer));

  default_config.set_frequency(kDefaultPumpFrequencyHz);

  sensor.set_connection_error_handler(base::Bind(
      &OrientationDevice::HandleSensorError, base::Unretained(this)));
  sensor->ConfigureReadingChangeNotifications(false /* disabled */);
  sensor->AddConfiguration(
      default_config, base::Bind(&OrientationDevice::OnSensorAddConfiguration,
                                 base::Unretained(this)));
}

void OrientationDevice::HandleSensorError() {
  VLOG(0) << "Handle errroe!";
  sensor.reset();
  shared_buffer_handle.reset();
  shared_buffer.reset();
}

// These functions should do nothing as they should never be called.
void OrientationDevice::RequestPresent(
    VRDisplayImpl* display,
    mojom::VRSubmitFrameClientPtr submit_client,
    mojom::VRPresentationProviderRequest request,
    mojom::VRDisplayHost::RequestPresentCallback callback) {
  NOTIMPLEMENTED();
};
void OrientationDevice::ExitPresent() {
  NOTIMPLEMENTED();
};

void OrientationDevice::GetPose(
    mojom::VRMagicWindowProvider::GetPoseCallback callback) {
  TRACE_EVENT1("gpu", "getPose", "isPresenting", false);

  SensorReading lastest_reading;
  Quaternion lastest_pose;

  shared_buffer_reader->GetReading(&lastest_reading);

  lastest_pose.set_x(lastest_reading.orientation_quat.x);
  lastest_pose.set_y(lastest_reading.orientation_quat.y);
  lastest_pose.set_z(lastest_reading.orientation_quat.z);
  lastest_pose.set_w(lastest_reading.orientation_quat.w);

  lastest_pose =
      WorldSpaceToUserOrientedSpace(SensorSpaceToWorldSpace(lastest_pose));

  mojom::VRPosePtr pose = mojom::VRPose::New();
  pose->orientation.emplace(4);

  pose->orientation.value()[0] = lastest_pose.x();
  pose->orientation.value()[1] = lastest_pose.y();
  pose->orientation.value()[2] = lastest_pose.z();
  pose->orientation.value()[3] = lastest_pose.w();

  std::move(callback).Run(std::move(pose));
}

// Mojo callback for Sensor::AddConfiguration().
void OrientationDevice::OnSensorAddConfiguration(bool success) {
  if (!success)
    HandleSensorError();
}

Quaternion OrientationDevice::SensorSpaceToWorldSpace(Quaternion q) {
  const display::Display::Rotation rotation =
      display::Screen::GetScreen()->GetPrimaryDisplay().rotation();

  if (rotation == display::Display::ROTATE_270) {
    // Swap tilt and yaw so that tilting the device up and down doesn't make it
    // go side to side.
    auto temp = q.x();
    q.set_x(q.y());
    q.set_y(-temp);
    q = Quaternion(Vector3dF(0, 0, 1), base::kPiDouble / 2) * q;
  } else if (rotation == display::Display::ROTATE_90) {
    // Swap tilt and yaw the other way
    auto temp = q.x();
    q.set_x(-q.y());
    q.set_y(temp);
    q = Quaternion(Vector3dF(0, 0, 1), -base::kPiDouble / 2) * q;
  }

  // Tilt the view up to have the y axis as the up down axis instead of z
  q = Quaternion(Vector3dF(1, 0, 0), -base::kPiDouble / 2) * q;

  return q;
}

Quaternion OrientationDevice::WorldSpaceToUserOrientedSpace(Quaternion q) {
  if (!have_base_pose_) {
    // A base pose to read the initial forward direction off of.
    have_base_pose_ = true;
    base_pose_ = q;

    // Extract the yaw from base pose to use as the base forward direction.
    base_pose_.set_x(0);
    base_pose_.set_z(0);
    base_pose_ = base_pose_.Normalized();
  }

  // Adjust the base forward on the orientation to where the original forward
  // was.
  q = base_pose_.inverse() * q;

  return q;
}

}  // namespace device