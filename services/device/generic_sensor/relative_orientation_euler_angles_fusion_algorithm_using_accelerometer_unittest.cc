// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/generic_sensor/relative_orientation_euler_angles_fusion_algorithm_using_accelerometer.h"
#include "services/device/generic_sensor/generic_sensor_consts.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

void verify_relative_orientation_euler_angles(
    double acceleration_x,
    double acceleration_y,
    double acceleration_z,
    double expected_alpha_in_degrees,
    double expected_beta_in_degrees,
    double expected_gamma_in_degrees) {
  double alpha_in_degrees, beta_in_degrees, gamma_in_degrees;

  RelativeOrientationEulerAnglesFusionAlgorithmUsingAccelerometer::
      ComputeRelativeOrientationFromAccelerometer(
          acceleration_x, acceleration_y, acceleration_z, &alpha_in_degrees,
          &beta_in_degrees, &gamma_in_degrees);

  EXPECT_DOUBLE_EQ(expected_alpha_in_degrees, alpha_in_degrees);
  EXPECT_DOUBLE_EQ(expected_beta_in_degrees, beta_in_degrees);
  EXPECT_DOUBLE_EQ(expected_gamma_in_degrees, gamma_in_degrees);
}

}  // namespace

// Tests a device resting flat.
TEST(RelativeOrientationEulerAnglesFusionAlgorithmUsingAccelerometerTest,
     NeutralOrientation) {
  double acceleration_x = 0.0;
  double acceleration_y = 0.0;
  double acceleration_z = kMeanGravity;
  double expected_alpha_in_degrees = 0.0;
  double expected_beta_in_degrees = 0.0;
  double expected_gamma_in_degrees = 0.0;

  verify_relative_orientation_euler_angles(
      acceleration_x, acceleration_y, acceleration_z, expected_alpha_in_degrees,
      expected_beta_in_degrees, expected_gamma_in_degrees);
}

// Tests an upside-down device, such that the W3C boundary [-180, 180) causes
// the beta value to become negative.
TEST(RelativeOrientationEulerAnglesFusionAlgorithmUsingAccelerometerTest,
     UpsideDown) {
  double acceleration_x = 0.0;
  double acceleration_y = 0.0;
  double acceleration_z = -kMeanGravity;
  double expected_alpha_in_degrees = 0.0;
  double expected_beta_in_degrees = -180.0;
  double expected_gamma_in_degrees = 0.0;

  verify_relative_orientation_euler_angles(
      acceleration_x, acceleration_y, acceleration_z, expected_alpha_in_degrees,
      expected_beta_in_degrees, expected_gamma_in_degrees);
}

// Tests a device lying on its left-edge.
TEST(RelativeOrientationEulerAnglesFusionAlgorithmUsingAccelerometerTest,
     LeftEdge) {
  double acceleration_x = kMeanGravity;
  double acceleration_y = 0.0;
  double acceleration_z = 0.0;
  double expected_alpha_in_degrees = 0.0;
  double expected_beta_in_degrees = 0.0;
  double expected_gamma_in_degrees = -90.0;

  verify_relative_orientation_euler_angles(
      acceleration_x, acceleration_y, acceleration_z, expected_alpha_in_degrees,
      expected_beta_in_degrees, expected_gamma_in_degrees);
}

// Tests a device lying on its right-edge, such that the W3C boundary [-90, 90)
// causes the gamma value to become negative.
TEST(RelativeOrientationEulerAnglesFusionAlgorithmUsingAccelerometerTest,
     RightEdge) {
  double acceleration_x = -kMeanGravity;
  double acceleration_y = 0.0;
  double acceleration_z = 0.0;
  double expected_alpha_in_degrees = 0.0;
  double expected_beta_in_degrees = 0.0;
  double expected_gamma_in_degrees = -90.0;

  verify_relative_orientation_euler_angles(
      acceleration_x, acceleration_y, acceleration_z, expected_alpha_in_degrees,
      expected_beta_in_degrees, expected_gamma_in_degrees);
}

}  // namespace device
