// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/generic_sensor/orientation_euler_angles_fusion_algorithm_using_quaternion.h"
#include "services/device/generic_sensor/orientation_test_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

constexpr double kEpsilon = 1e-8;

}  // namespace

TEST(OrientationEulerAnglesFusionAlgorithmUsingQuaternionTest,
     CheckSampleValues) {
  OrientationEulerAnglesFusionAlgorithmUsingQuaternion fusion_algorithm;

  ASSERT_EQ(quaternions_test_values.size(),
            euler_angles_in_degrees_test_values.size());

  for (size_t i = 0; i < quaternions_test_values.size(); ++i) {
    double x = quaternions_test_values[i][0];
    double y = quaternions_test_values[i][1];
    double z = quaternions_test_values[i][2];
    double w = quaternions_test_values[i][3];
    double alpha_in_degrees, beta_in_degrees, gamma_in_degrees;

    fusion_algorithm.ComputeEulerAnglesFromQuaternionForTest(
        x, y, z, w, &alpha_in_degrees, &beta_in_degrees, &gamma_in_degrees);

    EXPECT_NEAR(euler_angles_in_degrees_test_values[i][0], alpha_in_degrees,
                kEpsilon);
    EXPECT_NEAR(euler_angles_in_degrees_test_values[i][1], beta_in_degrees,
                kEpsilon);
    EXPECT_NEAR(euler_angles_in_degrees_test_values[i][2], gamma_in_degrees,
                kEpsilon);
  }
}

}  // namespace device
