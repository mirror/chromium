// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/viewport_aware_root.h"

#include <cmath>

#include "base/memory/ptr_util.h"
#include "chrome/browser/vr/test/animation_utils.h"
#include "chrome/browser/vr/ui_scene.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/insets_f.h"
#include "ui/gfx/transform.h"

namespace vr {

namespace {

const float threshold = ViewportAwareRoot::kViewportRotationTriggerDegrees;

bool MatricesAreNearlyEqual(const gfx::Transform& lhs,
                            const gfx::Transform& rhs) {
  float epsilon = 0.0001f;
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      if (std::abs(lhs.matrix().get(row, col) - rhs.matrix().get(row, col)) >
          epsilon)
        return false;
    }
  }
  return true;
}

void RotateAboutYAxis(float degrees, gfx::Vector3dF* out) {
  gfx::Transform transform;
  transform.RotateAboutYAxis(degrees);
  transform.TransformVector(out);
}

void CheckRotateClockwiseAndReverse(const gfx::Vector3dF& initial_look_at) {
  ViewportAwareRoot root;
  gfx::Vector3dF look_at(initial_look_at);
  gfx::Transform expected;

  root.AdjustPosition(look_at);
  EXPECT_TRUE(MatricesAreNearlyEqual(expected, root.LocalTransform()));

  float total_rotation = 0.f;
  RotateAboutYAxis(threshold - 1.f, &look_at);
  total_rotation += (threshold - 1.f);
  root.AdjustPosition(look_at);
  // Rotating less than threshold should yeild identity local transform.
  EXPECT_TRUE(MatricesAreNearlyEqual(expected, root.LocalTransform()));

  // Rotate look_at clockwise again threshold degrees.
  RotateAboutYAxis(threshold, &look_at);
  total_rotation += threshold;
  root.AdjustPosition(look_at);

  // Rotating more than threshold should reposition to element to center.
  // We have rotated threshold - 1 + threshold degrees in total.
  expected.RotateAboutYAxis(total_rotation);
  EXPECT_TRUE(MatricesAreNearlyEqual(expected, root.LocalTransform()));

  // Since we rotated, reset total rotation.
  total_rotation = 0.f;

  // Rotate look_at counter clockwise threshold-2 degrees.
  RotateAboutYAxis(-(threshold - 2.f), &look_at);
  total_rotation -= (threshold - 2.f);
  root.AdjustPosition(look_at);
  // Rotating opposite direction within the threshold should not reposition.
  EXPECT_TRUE(MatricesAreNearlyEqual(expected, root.LocalTransform()));

  // Rotate look_at counter clockwise again threshold degrees.
  RotateAboutYAxis(-threshold, &look_at);
  total_rotation -= threshold;
  root.AdjustPosition(look_at);
  expected.RotateAboutYAxis(total_rotation);
  // Rotating opposite direction passing the threshold should reposition.
  EXPECT_TRUE(MatricesAreNearlyEqual(expected, root.LocalTransform()));
}

}  // namespace

TEST(ViewportAwareRoot, TestAdjustPosition) {
  CheckRotateClockwiseAndReverse(gfx::Vector3dF{0.f, 0.f, -1.f});

  CheckRotateClockwiseAndReverse(
      gfx::Vector3dF{0.f, std::sin(1), -std::cos(1)});

  CheckRotateClockwiseAndReverse(
      gfx::Vector3dF{0.f, -std::sin(1.5), -std::cos(1.5)});
}

TEST(ViewportAwareRoot, ChildElementsRepositioned) {
  UiScene scene;

  auto root = base::MakeUnique<ViewportAwareRoot>();
  root->set_id(0);
  root->set_draw_phase(0);
  scene.AddUiElement(std::move(root));

  auto element = base::MakeUnique<UiElement>();
  element->set_id(1);
  element->set_viewport_aware(true);
  element->set_draw_phase(0);
  element->SetTranslate(0.f, 0.f, -1.f);
  scene.GetUiElementById(0)->AddChild(element.get());
  scene.AddUiElement(std::move(element));

  gfx::Vector3dF look_at{0.f, 0.f, -1.f};
  scene.OnBeginFrame(MicrosecondsToTicks(0), look_at);
  EXPECT_EQ(gfx::Point3F(0.f, 0.f, -1.f).ToString(),
            scene.GetUiElementById(1)->GetCenter().ToString());

  // This should trigger reposition of viewport aware elements.
  RotateAboutYAxis(90.f, &look_at);
  scene.OnBeginFrame(MicrosecondsToTicks(10), look_at);
  EXPECT_EQ(gfx::Point3F(-1.f, 0.f, -0.f).ToString(),
            scene.GetUiElementById(1)->GetCenter().ToString());
}

}  // namespace vr
