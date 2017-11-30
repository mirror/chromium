

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
static const std::string DEVICE_NAME = "Orientation Device";
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
    device::mojom::SensorProviderPtr& sensor_provider)
    : latest_pose_(), binding_(this) {
  sensor_provider->GetSensor(
      SensorType::ABSOLUTE_ORIENTATION_QUATERNION,
      base::Bind(&OrientationDevice::SensorReady, base::Unretained(this)));

  SetVRDisplayInfo(CreateVRDisplayInfo(GetId()));
}

OrientationDevice::~OrientationDevice() {}

void OrientationDevice::SensorReady(device::mojom::SensorInitParamsPtr params) {
  if (!params) {
    // This means that there are no orientation sensors on this device.
    HandleSensorError();
    std::move(ready_callback_).Run();
    return;
  }

  constexpr size_t kReadBufferSize = sizeof(device::SensorReadingSharedBuffer);

  DCHECK_EQ(0u, params->buffer_offset % kReadBufferSize);

  default_config_ = params->default_configuration;

  sensor_.Bind(std::move(params->sensor));

  binding_.Bind(std::move(params->client_request));

  shared_buffer_handle_ = std::move(params->memory);
  DCHECK(!shared_buffer_);
  shared_buffer_ = shared_buffer_handle_->MapAtOffset(kReadBufferSize,
                                                      params->buffer_offset);

  if (!shared_buffer_) {
    // If we cannot read data, we cannot supply a device.
    HandleSensorError();
    std::move(ready_callback_).Run();
    return;
  }

  const device::SensorReadingSharedBuffer* buffer =
      static_cast<const device::SensorReadingSharedBuffer*>(
          shared_buffer_.get());
  shared_buffer_reader_.reset(
      new device::SensorReadingSharedBufferReader(buffer));

  default_config_.set_frequency(kDefaultPumpFrequencyHz);

  sensor_.set_connection_error_handler(base::Bind(
      &OrientationDevice::HandleSensorError, base::Unretained(this)));
  sensor_->ConfigureReadingChangeNotifications(false /* disabled */);
  sensor_->AddConfiguration(
      default_config_, base::Bind(&OrientationDevice::OnSensorAddConfiguration,
                                  base::Unretained(this)));
}

// Mojo callback for Sensor::AddConfiguration().
void OrientationDevice::OnSensorAddConfiguration(bool success) {
  if (!success) {
    // Sensor config is not supported so we can't provide sensor events.
    HandleSensorError();
  } else {
    // We're good to go.
    available_ = true;
  }

  std::move(ready_callback_).Run();
}

void OrientationDevice::RaiseError() {
  HandleSensorError();
}

void OrientationDevice::HandleSensorError() {
  sensor_.reset();
  shared_buffer_handle_.reset();
  shared_buffer_.reset();
  binding_.Close();
}

void OrientationDevice::OnMagicWindowPoseRequest(
    mojom::VRMagicWindowProvider::GetPoseCallback callback) {
  mojom::VRPosePtr pose = mojom::VRPose::New();
  pose->orientation.emplace(4);

  SensorReading lastest_reading;
  // If the reading fails just return the last pose that we got.
  if (shared_buffer_reader_->GetReading(&lastest_reading)) {
    latest_pose_.set_x(lastest_reading.orientation_quat.x);
    latest_pose_.set_y(lastest_reading.orientation_quat.y);
    latest_pose_.set_z(lastest_reading.orientation_quat.z);
    latest_pose_.set_w(lastest_reading.orientation_quat.w);

    latest_pose_ =
        WorldSpaceToUserOrientedSpace(SensorSpaceToWorldSpace(latest_pose_));
  }

  pose->orientation.value()[0] = latest_pose_.x();
  pose->orientation.value()[1] = latest_pose_.y();
  pose->orientation.value()[2] = latest_pose_.z();
  pose->orientation.value()[3] = latest_pose_.w();

  std::move(callback).Run(std::move(pose));
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
  if (!base_pose_) {
    // Check that q is valid
    if (q.x() + q.y() + q.z() + q.w() == 0) {
      // q is invalid. Do not use for base pose.
      return q;
    }

    // A base pose to read the initial forward direction off of.
    base_pose_ = q;

    // Extract the yaw from base pose to use as the base forward direction.
    base_pose_->set_x(0);
    base_pose_->set_z(0);
    base_pose_ = base_pose_->Normalized();
  }

  // Adjust the base forward on the orientation to where the original forward
  // was.
  q = base_pose_->inverse() * q;

  return q;
}

bool OrientationDevice::IsFallbackDevice() {
  return true;
};

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

}  // namespace device