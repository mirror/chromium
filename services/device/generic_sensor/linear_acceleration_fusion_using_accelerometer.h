// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_GENERIC_SENSOR_LINEAR_ACCELERATION_FUSION_USING_ACCELEROMETER_H_
#define SERVICES_DEVICE_GENERIC_SENSOR_LINEAR_ACCELERATION_FUSION_USING_ACCELEROMETER_H_

#include "base/macros.h"
#include "services/device/generic_sensor/platform_sensor_fusion_algorithm.h"

namespace device {

// Algotithm that obtains linear acceleration values from data provided by
// accelerometer sensor. Simple low-pass filter is used to isolate gravity
// and substract it from accelerometer data to get linear acceleration.
class LinearAccelerationFusionUsingAccelerometer final
    : public PlatformSensorFusionAlgorithm {
 public:
  LinearAccelerationFusionUsingAccelerometer();
  ~LinearAccelerationFusionUsingAccelerometer() override;

  void GetFusedData(const std::vector<SensorReading>& readings,
                    SensorReading* fused_reading) override;
  void SetFrequency(double frequency) override;
  void Reset() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(LinearAccelerationFusionUsingAccelerometer);
  unsigned long reading_updates_count_;
  double time_constant_;
  double initial_timestamp_;
  double gravity_x_;
  double gravity_y_;
  double gravity_z_;
};

}  // namespace device

#endif  // SERVICES_DEVICE_GENERIC_SENSOR_LINEAR_ACCELERATION_FUSION_USING_ACCELEROMETER_H_
