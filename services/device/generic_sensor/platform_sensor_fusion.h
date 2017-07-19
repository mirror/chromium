// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_GENERIC_SENSOR_PLATFORM_SENSOR_FUSION_H_
#define SERVICES_DEVICE_GENERIC_SENSOR_PLATFORM_SENSOR_FUSION_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "services/device/generic_sensor/platform_sensor.h"
#include "services/device/generic_sensor/platform_sensor_provider_base.h"

namespace device {

class PlatformSensorFusionAlgorithm;

// Implementation of a platform sensor using fusion. This is a single instance
// object per browser process which is created by the singleton
// PlatformSensorProvider. If there are no clients, this instance is not
// created.
//
// The concept of fusion sensor is generic here. It is to implement a sensor
// using sensor data from one or more existing sensors. For example, it can be
// implementing *_EULER_ANGLES sensor using *_QUATERNION sensor, or vice-versa.
// It can also be implementing *_EULER_ANGLES sensor using ACCELEROMETER, etc.
class PlatformSensorFusion : public PlatformSensor,
                             public PlatformSensor::Client {
 public:
  // Construct a platform fusion sensor of type |fusion_sensor_type| using
  // one or more sensors whose sensor types are |source_sensor_types|, given
  // a buffer |mapping| where readings will be written.
  PlatformSensorFusion(
      mojo::ScopedSharedBufferMapping mapping,
      PlatformSensorProvider* provider,
      const PlatformSensorProviderBase::CreateSensorCallback& callback,
      const std::vector<mojom::SensorType>& source_sensor_types,
      mojom::SensorType fusion_sensor_type,
      std::unique_ptr<PlatformSensorFusionAlgorithm> fusion_algorithm);

  // PlatformSensor:
  mojom::ReportingMode GetReportingMode() override;
  PlatformSensorConfiguration GetDefaultConfiguration() override;
  // Can only be called once, the first time or after a StopSensor call.
  bool StartSensor(const PlatformSensorConfiguration& configuration) override;
  void StopSensor() override;
  bool CheckSensorConfiguration(
      const PlatformSensorConfiguration& configuration) override;

  // PlatformSensor::Client:
  void OnSensorReadingChanged() override;
  void OnSensorError() override;
  bool IsSuspended() override;

 protected:
  ~PlatformSensorFusion() override;

 private:
  void CreateSensorCallback(size_t index, scoped_refptr<PlatformSensor> sensor);
  void CreateSensorSucceeded();

  size_t num_sensors_created_ = 0;
  bool callback_called_ = false;
  PlatformSensorProviderBase::CreateSensorCallback callback_;
  SensorReading reading_;
  std::vector<scoped_refptr<PlatformSensor>> source_sensors_;
  std::unique_ptr<PlatformSensorFusionAlgorithm> fusion_algorithm_;

  DISALLOW_COPY_AND_ASSIGN(PlatformSensorFusion);
};

}  // namespace device

#endif  // SERVICES_DEVICE_GENERIC_SENSOR_PLATFORM_SENSOR_FUSION_H_
