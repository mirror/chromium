// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cmath>

#include "services/device/generic_sensor/absolute_orientation_euler_angles_fusion_algorithm_using_accelerometer_and_magnetometer.h"
#include "services/device/generic_sensor/orientation_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

constexpr double kEpsilon = 1e-8;

void verify_absolute_orientation_euler_angles(
    double gravity_x,
    double gravity_y,
    double gravity_z,
    double geomagnetic_x,
    double geomagnetic_y,
    double geomagnetic_z,
    double expected_alpha_in_degrees,
    double expected_beta_in_degrees,
    double expected_gamma_in_degrees) {
  double alpha_in_degrees, beta_in_degrees, gamma_in_degrees;

  EXPECT_TRUE(
      AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometer::
          ComputeAbsoluteOrientationEulerAnglesFromGravityAndGeomagnetic(
              gravity_x, gravity_y, gravity_z, geomagnetic_x, geomagnetic_y,
              geomagnetic_z, &alpha_in_degrees, &beta_in_degrees,
              &gamma_in_degrees));

  EXPECT_NEAR(expected_alpha_in_degrees, alpha_in_degrees, kEpsilon);
  EXPECT_NEAR(expected_beta_in_degrees, beta_in_degrees, kEpsilon);
  EXPECT_NEAR(expected_gamma_in_degrees, gamma_in_degrees, kEpsilon);
}

}  // namespace

TEST(
    AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometerTest,
    GravityLessThanTenPercentOfNormalValue) {
  double gravity_x = 0.1;
  double gravity_y = 0.2;
  double gravity_z = 0.3;
  double geomagnetic_x = 1.0;
  double geomagnetic_y = 2.0;
  double geomagnetic_z = 3.0;
  double alpha_in_degrees, beta_in_degrees, gamma_in_degrees;

  EXPECT_FALSE(
      AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometer::
          ComputeAbsoluteOrientationEulerAnglesFromGravityAndGeomagnetic(
              gravity_x, gravity_y, gravity_z, geomagnetic_x, geomagnetic_y,
              geomagnetic_z, &alpha_in_degrees, &beta_in_degrees,
              &gamma_in_degrees));
}

TEST(
    AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometerTest,
    NormalHIsTooSmall) {
  double gravity_x = 1.0;
  double gravity_y = 1.0;
  double gravity_z = 1.0;
  double geomagnetic_x = 1.0;
  double geomagnetic_y = 1.0;
  double geomagnetic_z = 1.0;
  double alpha_in_degrees, beta_in_degrees, gamma_in_degrees;

  EXPECT_FALSE(
      AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometer::
          ComputeAbsoluteOrientationEulerAnglesFromGravityAndGeomagnetic(
              gravity_x, gravity_y, gravity_z, geomagnetic_x, geomagnetic_y,
              geomagnetic_z, &alpha_in_degrees, &beta_in_degrees,
              &gamma_in_degrees));
}

TEST(
    AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometerTest,
    Identity) {
  double gravity_x = 0.0;
  double gravity_y = 0.0;
  double gravity_z = 1.0;
  double geomagnetic_x = 0.0;
  double geomagnetic_y = 1.0;
  double geomagnetic_z = 0.0;

  double expected_alpha_in_degrees = 0.0;
  double expected_beta_in_degrees = 0.0;
  double expected_gamma_in_degrees = 0.0;

  verify_absolute_orientation_euler_angles(
      gravity_x, gravity_y, gravity_z, geomagnetic_x, geomagnetic_y,
      geomagnetic_z, expected_alpha_in_degrees, expected_beta_in_degrees,
      expected_gamma_in_degrees);
}

TEST(
    AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometerTest,
    BetaIs45Degrees) {
  double gravity_x = 0.0;
  double gravity_y = std::sin(kDegToRad * 45.0);
  double gravity_z = std::cos(kDegToRad * 45.0);
  double geomagnetic_x = 0.0;
  double geomagnetic_y = 1.0;
  double geomagnetic_z = 0.0;

  double expected_alpha_in_degrees = 0.0;
  double expected_beta_in_degrees = 45.0;
  double expected_gamma_in_degrees = 0.0;

  verify_absolute_orientation_euler_angles(
      gravity_x, gravity_y, gravity_z, geomagnetic_x, geomagnetic_y,
      geomagnetic_z, expected_alpha_in_degrees, expected_beta_in_degrees,
      expected_gamma_in_degrees);
}

TEST(
    AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometerTest,
    GammaIs45Degrees) {
  double gravity_x = -std::sin(kDegToRad * 45.0);
  double gravity_y = 0.0;
  double gravity_z = std::cos(kDegToRad * 45.0);
  double geomagnetic_x = 0.0;
  double geomagnetic_y = 1.0;
  double geomagnetic_z = 0.0;

  double expected_alpha_in_degrees = 0.0;
  double expected_beta_in_degrees = 0.0;
  double expected_gamma_in_degrees = 45.0;

  verify_absolute_orientation_euler_angles(
      gravity_x, gravity_y, gravity_z, geomagnetic_x, geomagnetic_y,
      geomagnetic_z, expected_alpha_in_degrees, expected_beta_in_degrees,
      expected_gamma_in_degrees);
}

TEST(
    AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometerTest,
    AlphaIs45Degrees) {
  double gravity_x = 0.0;
  double gravity_y = 0.0;
  double gravity_z = 1.0;
  double geomagnetic_x = std::sin(kDegToRad * 45.0);
  double geomagnetic_y = std::cos(kDegToRad * 45.0);
  double geomagnetic_z = 0.0;

  double expected_alpha_in_degrees = 45.0;
  double expected_beta_in_degrees = 0.0;
  double expected_gamma_in_degrees = 0.0;

  verify_absolute_orientation_euler_angles(
      gravity_x, gravity_y, gravity_z, geomagnetic_x, geomagnetic_y,
      geomagnetic_z, expected_alpha_in_degrees, expected_beta_in_degrees,
      expected_gamma_in_degrees);
}

TEST(
    AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometerTest,
    GimbalLock) {
  double gravity_x = 0.0;
  double gravity_y = 1.0;
  double gravity_z = 0.0;
  double geomagnetic_x = std::sin(kDegToRad * 45.0);
  double geomagnetic_y = 0.0;
  double geomagnetic_z = -std::cos(kDegToRad * 45.0);

  // Favor Alpha instead of Gamma.
  double expected_alpha_in_degrees = 45.0;
  double expected_beta_in_degrees = 90.0;
  double expected_gamma_in_degrees = 0.0;

  verify_absolute_orientation_euler_angles(
      gravity_x, gravity_y, gravity_z, geomagnetic_x, geomagnetic_y,
      geomagnetic_z, expected_alpha_in_degrees, expected_beta_in_degrees,
      expected_gamma_in_degrees);
}

TEST(
    AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometerTest,
    BetaIsGreaterThan90Degrees) {
  double gravity_x = 0.0;
  double gravity_y = std::cos(kDegToRad * 45.0);
  double gravity_z = -std::sin(kDegToRad * 45.0);
  double geomagnetic_x = 0.0;
  double geomagnetic_y = 0.0;
  double geomagnetic_z = -1.0;

  double expected_alpha_in_degrees = 0.0;
  double expected_beta_in_degrees = 135.0;
  double expected_gamma_in_degrees = 0.0;

  verify_absolute_orientation_euler_angles(
      gravity_x, gravity_y, gravity_z, geomagnetic_x, geomagnetic_y,
      geomagnetic_z, expected_alpha_in_degrees, expected_beta_in_degrees,
      expected_gamma_in_degrees);
}

TEST(
    AbsoluteOrientationEulerAnglesFusionAlgorithmUsingAccelerometerAndMagnetometerTest,
    GammaIsMinus90Degrees) {
  double gravity_x = -1.0;
  double gravity_y = 0.0;
  double gravity_z = 0.0;
  double geomagnetic_x = 0.0;
  double geomagnetic_y = 1.0;
  double geomagnetic_z = 0.0;

  double expected_alpha_in_degrees = 180.0;
  double expected_beta_in_degrees = -180.0;
  double expected_gamma_in_degrees = -90.0;

  verify_absolute_orientation_euler_angles(
      gravity_x, gravity_y, gravity_z, geomagnetic_x, geomagnetic_y,
      geomagnetic_z, expected_alpha_in_degrees, expected_beta_in_degrees,
      expected_gamma_in_degrees);
}

}  // namespace device
