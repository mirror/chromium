// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/device_sensors/device_orientation_event_pump.h"

#include <cmath>

#include "content/public/renderer/render_frame.h"
#include "content/renderer/render_thread_impl.h"
#include "services/device/public/interfaces/sensor.mojom.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace {

bool IsAngleDifferent(bool has_angle1,
                      double angle1,
                      bool has_angle2,
                      double angle2) {
  if (has_angle1 != has_angle2)
    return true;

  return (has_angle1 &&
          std::fabs(angle1 - angle2) >=
              content::DeviceOrientationEventPump::kOrientationThreshold);
}

bool IsSignificantlyDifferent(const device::OrientationData& data1,
                              const device::OrientationData& data2) {
  return IsAngleDifferent(data1.has_alpha, data1.alpha, data2.has_alpha,
                          data2.alpha) ||
         IsAngleDifferent(data1.has_beta, data1.beta, data2.has_beta,
                          data2.beta) ||
         IsAngleDifferent(data1.has_gamma, data1.gamma, data2.has_gamma,
                          data2.gamma);
}

}  // namespace

namespace content {

const double DeviceOrientationEventPump::kOrientationThreshold = 0.1;

DeviceOrientationEventPump::DeviceOrientationEventPump(RenderThread* thread,
                                                       bool absolute)
    : DeviceSensorEventPump<blink::WebDeviceOrientationListener>(thread),
      relative_orientation_sensor_(
          this,
          device::mojom::SensorType::RELATIVE_ORIENTATION_EULER_ANGLES),
      absolute_orientation_sensor_(
          this,
          device::mojom::SensorType::ABSOLUTE_ORIENTATION_EULER_ANGLES),
      absolute_(absolute) {}

DeviceOrientationEventPump::~DeviceOrientationEventPump() {}

void DeviceOrientationEventPump::SendStartMessage() {
  // When running layout tests, those observers should not listen to the
  // actual hardware changes. In order to make that happen, don't connect
  // the other end of the mojo pipe to anything.
  //
  // TODO(sammc): Remove this when JS layout test support for shared buffers
  // is ready and the layout tests are converted to use that for mocking.
  if (!RenderThreadImpl::current() ||
      RenderThreadImpl::current()->layout_test_mode()) {
    return;
  }

  if (!absolute_orientation_sensor_.sensor &&
      !relative_orientation_sensor_.sensor) {
    if (!sensor_provider_) {
      RenderFrame* const render_frame = GetRenderFrame();
      if (!render_frame)
        return;

      CHECK(render_frame->GetRemoteInterfaces());

      render_frame->GetRemoteInterfaces()->GetInterface(
          mojo::MakeRequest(&sensor_provider_));
      sensor_provider_.set_connection_error_handler(
          base::Bind(&DeviceSensorEventPump::HandleSensorProviderError,
                     base::Unretained(this)));
    }
    GetSensor(&absolute_orientation_sensor_);
    if (!absolute_)
      GetSensor(&relative_orientation_sensor_);
  } else {
    if (relative_orientation_sensor_.sensor)
      relative_orientation_sensor_.sensor->Resume();

    if (absolute_orientation_sensor_.sensor)
      absolute_orientation_sensor_.sensor->Resume();

    DidStartIfPossible();
  }
}

void DeviceOrientationEventPump::SendStopMessage() {
  // SendStopMessage() gets called both when the page visibility changes and if
  // all device orientation event listeners are unregistered. Since removing
  // the event listener is more rare than the page visibility changing,
  // Sensor::Suspend() is used to optimize this case for not doing extra work.
  if (relative_orientation_sensor_.sensor)
    relative_orientation_sensor_.sensor->Suspend();

  if (absolute_orientation_sensor_.sensor)
    absolute_orientation_sensor_.sensor->Suspend();
}

void DeviceOrientationEventPump::SendFakeDataForTesting(void* fake_data) {
  device::OrientationData data =
      *static_cast<device::OrientationData*>(fake_data);
  listener()->DidChangeDeviceOrientation(data);
}

void DeviceOrientationEventPump::FireEvent() {
  device::OrientationData data;
  data.absolute = absolute_;

  DCHECK(listener());

  GetDataFromSharedMemory(&data);

  if (ShouldFireEvent(data)) {
    data_ = data;
    listener()->DidChangeDeviceOrientation(data);
  }
}

bool DeviceOrientationEventPump::SensorSharedBuffersReady() const {
  if (relative_orientation_sensor_.sensor &&
      !relative_orientation_sensor_.shared_buffer)
    return false;

  if (absolute_orientation_sensor_.sensor &&
      !absolute_orientation_sensor_.shared_buffer) {
    return false;
  }

  return true;
}

void DeviceOrientationEventPump::GetDataFromSharedMemory(
    device::OrientationData* data) {
  if (!absolute_ && relative_orientation_sensor_.SensorReadingCouldBeRead()) {
    // For DeviceOrientation Event, this provides relative orientation data.
    data->alpha =
        relative_orientation_sensor_.reading.orientation_euler.z;  // z: alpha
    data->beta =
        relative_orientation_sensor_.reading.orientation_euler.x;  // x: beta
    data->gamma =
        relative_orientation_sensor_.reading.orientation_euler.y;  // y: gamma
    data->has_alpha = true;
    data->has_beta = true;
    data->has_gamma = true;
    data->absolute = false;
  } else if (absolute_orientation_sensor_.SensorReadingCouldBeRead()) {
    // For DeviceOrientationAbsolute Event, this provides absolute orientation
    // data.
    //
    // For DeviceOrientation Event, this provide absolute orientation data if
    // relative orientation data is not available.
    data->alpha =
        absolute_orientation_sensor_.reading.orientation_euler.z;  // z: alpha
    data->beta =
        absolute_orientation_sensor_.reading.orientation_euler.x;  // x: beta
    data->gamma =
        absolute_orientation_sensor_.reading.orientation_euler.y;  // y: gamma
    data->has_alpha = true;
    data->has_beta = true;
    data->has_gamma = true;
    data->absolute = true;
  }
}

bool DeviceOrientationEventPump::ShouldFireEvent(
    const device::OrientationData& data) const {
  if (!data.has_alpha && !data.has_beta && !data.has_gamma) {
    // no data can be provided, this is an all-null event.
    return true;
  }

  return IsSignificantlyDifferent(data_, data);
}

}  // namespace content
