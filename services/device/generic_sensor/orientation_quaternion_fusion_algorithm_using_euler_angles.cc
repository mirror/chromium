// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/generic_sensor/orientation_quaternion_fusion_algorithm_using_euler_angles.h"

#include <cmath>

#include "base/logging.h"
#include "services/device/generic_sensor/orientation_util.h"
#include "services/device/generic_sensor/platform_sensor_fusion.h"

namespace {

void ComputeQuaternionFromEulerAngles(double alpha_in_degrees,
                                      double beta_in_degrees,
                                      double gamma_in_degrees,
                                      double* x,
                                      double* y,
                                      double* z,
                                      double* w) {
  double alpha_radians = device::kDeg2rad * alpha_in_degrees;
  double beta_radians = device::kDeg2rad * beta_in_degrees;
  double gamma_radians = device::kDeg2rad * gamma_in_degrees;

  double cx = std::cos(beta_radians / 2);
  double cy = std::cos(gamma_radians / 2);
  double cz = std::cos(alpha_radians / 2);
  double sx = std::sin(beta_radians / 2);
  double sy = std::sin(gamma_radians / 2);
  double sz = std::sin(alpha_radians / 2);

  *x = sx * cy * cz - cx * sy * sz;
  *y = cx * sy * cz + sx * cy * sz;
  *z = cx * cy * sz + sx * sy * cz;
  *w = cx * cy * cz - sx * sy * sz;
}

}  // namespace

namespace device {

OrientationQuaternionFusionAlgorithmUsingEulerAngles::
    OrientationQuaternionFusionAlgorithmUsingEulerAngles() {}

OrientationQuaternionFusionAlgorithmUsingEulerAngles::
    ~OrientationQuaternionFusionAlgorithmUsingEulerAngles() = default;

bool OrientationQuaternionFusionAlgorithmUsingEulerAngles::GetFusedData(
    mojom::SensorType which_sensor_changed,
    SensorReading* fused_reading) {
  // Transform the *_ORIENTATION_EULER_ANGLES values to
  // *_ORIENTATION_QUATERNION.
  DCHECK(fusion_sensor_);

  SensorReading reading;
  if (!fusion_sensor_->GetLatestReading(0, &reading))
    return false;

  double beta_in_degrees = reading.orientation_euler.x;
  double gamma_in_degrees = reading.orientation_euler.y;
  double alpha_in_degrees = reading.orientation_euler.z;
  double x, y, z, w;
  ComputeQuaternionFromEulerAngles(alpha_in_degrees, beta_in_degrees,
                                   gamma_in_degrees, &x, &y, &z, &w);
  fused_reading->orientation_quat.x = x;
  fused_reading->orientation_quat.y = y;
  fused_reading->orientation_quat.z = z;
  fused_reading->orientation_quat.w = w;

  return true;
}

void OrientationQuaternionFusionAlgorithmUsingEulerAngles::
    ComputeQuaternionFromEulerAnglesForTest(double alpha_in_degrees,
                                            double beta_in_degrees,
                                            double gamma_in_degrees,
                                            double* x,
                                            double* y,
                                            double* z,
                                            double* w) {
  ComputeQuaternionFromEulerAngles(alpha_in_degrees, beta_in_degrees,
                                   gamma_in_degrees, x, y, z, w);
}

}  // namespace device
