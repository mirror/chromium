// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/device/generic_sensor/orientation_quaternion_fusion_algorithm_using_euler_angles.h"
#include "services/device/generic_sensor/orientation_test_data.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

namespace {

const double kEpsilon = 1e-8;

}  // namespace

TEST(OrientationQuaternionFusionAlgorithmUsingEulerAnglesTest,
     CheckSampleValues) {
  OrientationQuaternionFusionAlgorithmUsingEulerAngles fusion_algorithm;

  ASSERT_EQ(euler_angles_in_degrees_test_values.size(),
            quaternions_test_values.size());

  for (size_t i = 0; i < euler_angles_in_degrees_test_values.size(); ++i) {
    double alpha_in_degrees = euler_angles_in_degrees_test_values[i][0];
    double beta_in_degrees = euler_angles_in_degrees_test_values[i][1];
    double gamma_in_degrees = euler_angles_in_degrees_test_values[i][2];
    double x, y, z, w;

    fusion_algorithm.ComputeQuaternionFromEulerAnglesForTest(
        alpha_in_degrees, beta_in_degrees, gamma_in_degrees, &x, &y, &z, &w);

    EXPECT_NEAR(quaternions_test_values[i][0], x, kEpsilon);
    EXPECT_NEAR(quaternions_test_values[i][1], y, kEpsilon);
    EXPECT_NEAR(quaternions_test_values[i][2], z, kEpsilon);
    EXPECT_NEAR(quaternions_test_values[i][3], w, kEpsilon);

    EXPECT_NEAR(1.0, x * x + y * y + z * z + w * w, kEpsilon);
  }
}

}  // namespace device
