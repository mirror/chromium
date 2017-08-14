// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/generic_sensor/relative_orientation_euler_angles_fusion_algorithm_using_accelerometer.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

constexpr double kEpsilon = 1e-8;

constexpr double kAngleForTest = 5.852716538156;

// The computed relative orientation value of an entry in
// |accelerometer_test_values| is stored in the same index in
// |relative_orientation_test_values|.

// The values in each three-element entry are: acceleration_x, acceleration_y,
// acceleration_z.
const std::vector<std::vector<double>> accelerometer_test_values = {
    {0.0, 0.0, 0.0},   {0.0, 0.0, 1.0},   {0.0, 0.0, -1.0},  {0.0, 1.0, 0.0},
    {0.0, 1.0, 1.0},   {0.0, 1.0, -1.0},  {0.0, -1.0, 0.0},  {0.0, -1.0, 1.0},
    {0.0, -1.0, -1.0}, {1.0, 0.0, 0.0},   {1.0, 0.0, 1.0},   {1.0, 0.0, -1.0},
    {1.0, 1.0, 0.0},   {1.0, 1.0, 1.0},   {1.0, 1.0, -1.0},  {1.0, -1.0, 0.0},
    {1.0, -1.0, 1.0},  {1.0, -1.0, -1.0}, {-1.0, 0.0, 0.0},  {-1.0, 0.0, 1.0},
    {-1.0, 0.0, -1.0}, {-1.0, 1.0, 0.0},  {-1.0, 1.0, 1.0},  {-1.0, 1.0, -1.0},
    {-1.0, -1.0, 0.0}, {-1.0, -1.0, 1.0}, {-1.0, -1.0, -1.0}};

// The values in each three-element entry are: alpha, beta, gamma.
const std::vector<std::vector<double>> relative_orientation_test_values = {
    {0.0, 0.0, 0.0},
    {0.0, 0.0, 0.0},
    {0.0, -180.0, 0.0},
    {0.0, -90.0, 0.0},
    {0.0, -45.0, 0.0},
    {0.0, -135.0, 0.0},
    {0.0, 90.0, 0.0},
    {0.0, 45.0, 0.0},
    {0.0, 135.0, 0.0},
    {0.0, 0.0, kAngleForTest},
    {0.0, 0.0, kAngleForTest},
    {0.0, -180.0, kAngleForTest},
    {0.0, -90.0, kAngleForTest},
    {0.0, -45.0, kAngleForTest},
    {0.0, -135.0, kAngleForTest},
    {0.0, 90.0, kAngleForTest},
    {0.0, 45.0, kAngleForTest},
    {0.0, 135.0, kAngleForTest},
    {0.0, 0.0, -kAngleForTest},
    {0.0, 0.0, -kAngleForTest},
    {0.0, -180.0, -kAngleForTest},
    {0.0, -90.0, -kAngleForTest},
    {0.0, -45.0, -kAngleForTest},
    {0.0, -135.0, -kAngleForTest},
    {0.0, 90.0, -kAngleForTest},
    {0.0, 45.0, -kAngleForTest},
    {0.0, 135.0, -kAngleForTest}};

}  // namespace

TEST(RelativeOrientationEulerAnglesFusionAlgorithmUsingAccelerometerTest,
     CheckSampleValues) {
  ASSERT_EQ(accelerometer_test_values.size(),
            relative_orientation_test_values.size());

  for (size_t i = 0; i < accelerometer_test_values.size(); ++i) {
    double acceleration_x = accelerometer_test_values[i][0];
    double acceleration_y = accelerometer_test_values[i][1];
    double acceleration_z = accelerometer_test_values[i][2];
    double alpha_in_degrees, beta_in_degrees, gamma_in_degrees;

    RelativeOrientationEulerAnglesFusionAlgorithmUsingAccelerometer::
        ComputeRelativeOrientationFromAccelerometer(
            acceleration_x, acceleration_y, acceleration_z, &alpha_in_degrees,
            &beta_in_degrees, &gamma_in_degrees);

    EXPECT_NEAR(relative_orientation_test_values[i][0], alpha_in_degrees,
                kEpsilon);
    EXPECT_NEAR(relative_orientation_test_values[i][1], beta_in_degrees,
                kEpsilon);
    EXPECT_NEAR(relative_orientation_test_values[i][2], gamma_in_degrees,
                kEpsilon);
  }
}

}  // namespace device
