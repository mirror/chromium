// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_DEVICE_GENERIC_SENSOR_ABSOLUTE_ORIENTATION_EULER_ANGLES_FUSION_ALGORITHM_USING_ACCELEROMETER_AND_MAGNETOMETER_H_
#define SERVICES_DEVICE_GENERIC_SENSOR_ABSOLUTE_ORIENTATION_EULER_ANGLES_FUSION_ALGORITHM_USING_ACCELEROMETER_AND_MAGNETOMETER_H_

#include "base/macros.h"
#include "services/device/generic_sensor/platform_sensor_fusion_algorithm.h"

namespace device {

// Sensor fusion algorithm for implementing ABSOLUTE_ORIENTATION_EULER_ANGLES
// using ACCELEROMETER and MAGNETOMETER.
class
    AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometer
    : public PlatformSensorFusionAlgorithm {
 public:
  AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometer();
  ~AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometer()
      override;

  static bool ComputeAbsoluteOrientationEulerAnglesFromGravityAndGeomagnetic(
      double gravity_x,
      double gravity_y,
      double gravity_z,
      double geomagnetic_x,
      double geomagnetic_y,
      double geomagnetic_z,
      double* alpha_in_degrees,
      double* beta_in_degrees,
      double* gamma_in_degrees);

  bool GetFusedData(mojom::SensorType which_sensor_changed,
                    SensorReading* fused_reading) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(
      AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometer);
};

}  // namespace device

#endif  // SERVICES_DEVICE_GENERIC_SENSOR_ABSOLUTE_ORIENTATION_EULER_ANGLES_FUSION_ALGORITHM_USING_ACCELEROMETER_AND_MAGNETOMETER_H_
