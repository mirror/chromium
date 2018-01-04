// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/repositioner.h"

#include "base/memory/ptr_util.h"
#include "cc/test/geometry_test_utils.h"
#include "chrome/browser/vr/test/animation_utils.h"
#include "chrome/browser/vr/test/constants.h"
#include "chrome/browser/vr/ui_scene.h"
#include "chrome/browser/vr/ui_scene_constants.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/transform.h"

namespace vr {

namespace {

constexpr float kTestRepositionDistance = 2.f;

void CheckRepositionedCorrectly(UiElement* element,
                                const gfx::Point3F& expected_center) {
  gfx::Point3F center = element->GetCenter();
  EXPECT_NEAR(center.x(), expected_center.x(), kEpsilon);
  EXPECT_NEAR(center.y(), expected_center.y(), kEpsilon);
  EXPECT_NEAR(center.z(), expected_center.z(), kEpsilon);
  gfx::Vector3dF right_vector = {1, 0, 0};
  element->world_space_transform().TransformVector(&right_vector);
  gfx::Vector3dF expected_right =
      gfx::CrossProduct(expected_center - kOrigin, {0, 1, 0});
  gfx::Vector3dF normalized_expected_right_vector;
  expected_right.GetNormalized(&normalized_expected_right_vector);
  EXPECT_VECTOR3DF_NEAR(right_vector, normalized_expected_right_vector,
                        kEpsilon);
}

}  // namespace

TEST(Repositioner, RepositionNegativeZWithReticle) {
  UiScene scene;
  auto child = base::MakeUnique<UiElement>();
  child->SetTranslate(0, 0, -kTestRepositionDistance);
  auto* element = child.get();
  auto parent = base::MakeUnique<Repositioner>(kTestRepositionDistance);
  auto* repositioner = parent.get();
  parent->AddChild(std::move(child));
  scene.AddUiElement(kRoot, std::move(parent));

  struct TestCase {
    gfx::Vector3dF laser_direction;
    gfx::Point3F expected_element_center;
  };
  std::vector<TestCase> test_cases = {
      {{-2, 2.41421, -1}, {-1, 1.41421, -1}},
      {{-2, 2.41421, 1}, {-1, 1.41421, 1}},
      {{0, 2.41421, 1}, {1, 1.41421, 1}},
      {{0, 2.41421, -1}, {1, 1.41421, -1}},
  };

  repositioner->set_laser_origin(gfx::Point3F(1, -1, 0));

  for (const auto& test_case : test_cases) {
    repositioner->set_laser_direction(test_case.laser_direction);
    scene.OnBeginFrame(MsToTicks(0), kForwardVector);
    // Before enable repositioner, child element should NOT have rotation.
    CheckRepositionedCorrectly(element, {0, 0, -kTestRepositionDistance});
  }

  repositioner->set_enable(true);

  for (const auto& test_case : test_cases) {
    repositioner->set_laser_direction(test_case.laser_direction);
    scene.OnBeginFrame(MsToTicks(0), kForwardVector);
    CheckRepositionedCorrectly(element, test_case.expected_element_center);
  }
}

}  // namespace vr
