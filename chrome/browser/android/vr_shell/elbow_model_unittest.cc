// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/vr_shell/elbow_model.h"

#include "testing/gtest/include/gtest/gtest.h"

#define TOLERANCE 0.0001

#define EXPECT_POINT3F_NEAR(a, b)       \
  EXPECT_NEAR(a.x(), b.x(), TOLERANCE); \
  EXPECT_NEAR(a.y(), b.y(), TOLERANCE); \
  EXPECT_NEAR(a.z(), b.z(), TOLERANCE);

#define EXPECT_QUAT_NEAR(a, b)        \
  EXPECT_NEAR(a.qx, b.qx, TOLERANCE); \
  EXPECT_NEAR(a.qy, b.qy, TOLERANCE); \
  EXPECT_NEAR(a.qz, b.qz, TOLERANCE); \
  EXPECT_NEAR(a.qw, b.qw, TOLERANCE);

namespace vr_shell {

namespace {

struct ElbowModelState {
  gvr::ControllerHandedness handedness;
  gfx::Point3F wrist_position;
  vr::Quatf wrist_rotation;
  float alpha_value;
  gfx::Point3F elbow_position;
  vr::Quatf elbow_rotation;
  gfx::Point3F shoulder_position;
  vr::Quatf shoulder_rotation;
  gfx::Vector3dF torso_direction;
  gfx::Vector3dF handed_multiplier;
};

class TestElbowModel : public ElbowModel {
 public:
  explicit TestElbowModel(const ElbowModelState& state)
      : ElbowModel(state.handedness) {
    wrist_position_ = state.wrist_position;
    wrist_rotation_ = state.wrist_rotation;
    alpha_value_ = state.alpha_value;
    elbow_position_ = state.elbow_position;
    elbow_rotation_ = state.elbow_rotation;
    shoulder_position_ = state.shoulder_position;
    shoulder_rotation_ = state.shoulder_rotation;
    torso_direction_ = state.torso_direction;
    handed_multiplier_ = state.handed_multiplier;
  }
};

TEST(ElbowModelTest, Update) {
  struct {
    ElbowModelState state;
    ElbowModel::UpdateData update;
    gfx::Point3F expected_controller_position;
    vr::Quatf expected_controller_rotation;
  } test_cases[] = {
      {{GVR_CONTROLLER_RIGHT_HANDED,
        {-0.049917, -0.505498, 0.038013},
        {0.076829, -0.920871, 0.022864, -0.381537},
        1.000000,
        {0.198562, -0.500000, 0.064984},
        {0.055264, -0.665585, 0.045162, -0.742902},
        {-0.117488, -0.190000, 0.152280},
        {0.000000, 0.025441, 0.000000, 0.999676},
        {-0.050865, 0.000000, -0.998706},
        {1.000000, 1.000000, 1.000000}},
       {true,
        {0.046834, -0.922304, 0.009723, -0.383494},
        {-0.008729, -0.148389, 4.381846},
        {-0.040660, -0.587623, -0.808112},
        0.016145},
       {-0.049856, -0.505970, 0.037537},
       {0.046834, -0.922304, 0.009723, -0.383494}},

      {{GVR_CONTROLLER_RIGHT_HANDED,
        {-0.049500, -0.505613, 0.036904},
        {0.002509, -0.923641, -0.008170, -0.383164},
        1.000000,
        {0.199006, -0.500000, 0.063613},
        {0.001509, -0.668267, -0.015119, -0.743766},
        {-0.064551, -0.190000, 0.181179},
        {0.000000, 0.028889, 0.000000, 0.999583},
        {-0.057754, 0.000000, -0.998331},
        {1.000000, 1.000000, 1.000000}},
       {true,
        {0.030375, -0.923703, 0.003814, -0.381883},
        {0.000000, 0.104745, -3.954136},
        {-0.089099, -0.617505, -0.781504},
        0.016437},
       {-0.049306, -0.505320, 0.036859},
       {0.030375, -0.923704, 0.003814, -0.381883}},

      {{GVR_CONTROLLER_RIGHT_HANDED,
        {-0.048024, -0.505038, 0.033704},
        {0.090189, -0.919197, 0.028959, -0.382237},
        1.000000,
        {0.200756, -0.500000, 0.057853},
        {0.065221, -0.668916, 0.056922, -0.738280},
        {0.123319, -0.190000, 0.147598},
        {0.000000, 0.043287, 0.000000, 0.999063},
        {-0.086493, 0.000000, -0.996252},
        {1.000000, 1.000000, 1.000000}},
       {true,
        {0.089167, -0.918995, 0.028835, -0.382971},
        {0.008729, -0.069830, -0.942708},
        {-0.095425, -0.619931, -0.778832},
        0.016381},
       {-0.047974, -0.504896, 0.033391},
       {0.089167, -0.918995, 0.028835, -0.382971}},
  };

  for (size_t i = 0; i < arraysize(test_cases); ++i) {
    TestElbowModel elbow_model(test_cases[i].state);
    elbow_model.Update(test_cases[i].update);
    const gfx::Point3F& p = elbow_model.GetControllerPosition();
    const vr::Quatf& q = elbow_model.GetControllerRotation();
    EXPECT_POINT3F_NEAR(test_cases[i].expected_controller_position, p);
    EXPECT_QUAT_NEAR(test_cases[i].expected_controller_rotation, q);
  }
}

}  // namespace

}  // namespace vr_shell
