// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/elements/ui_element.h"

#include <utility>

#include "base/macros.h"
#include "cc/animation/animation.h"
#include "cc/animation/keyframed_animation_curve.h"
#include "cc/test/geometry_test_utils.h"
#include "chrome/browser/vr/test/animation_creator.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace vr {

TEST(UiElements, AnimateSize) {
  UiElement rect;
  AnimationCreator creator;
  rect.set_size({10, 100, 1});
  creator.set_from_size({10, 100});
  creator.set_to_size({20, 200});
  rect.animation_player().AddAnimation(creator.size_animation(1, 1));
  base::TimeTicks start_time = AnimationCreator::usToTicks(1);
  rect.Animate(start_time);
  EXPECT_VECTOR3DF_EQ(gfx::Vector3dF(10, 100, 1), rect.size());
  rect.Animate(start_time + AnimationCreator::usToDelta(10000));
  EXPECT_VECTOR3DF_EQ(gfx::Vector3dF(20, 200, 1), rect.size());
}

TEST(UiElements, AnimationAffectsInheritableTransform) {
  UiElement rect;
  AnimationCreator creator;
  creator.from_operations().AppendTranslate(10, 100, 1000);
  creator.to_operations().AppendTranslate(20, 200, 2000);
  rect.animation_player().AddAnimation(creator.transform_animation(1, 1));
  base::TimeTicks start_time = AnimationCreator::usToTicks(1);
  rect.Animate(start_time);
  gfx::Point3F p;
  rect.transform_operations().Apply().TransformPoint(&p);
  EXPECT_VECTOR3DF_EQ(gfx::Vector3dF(10, 100, 1000), p);
  p = gfx::Point3F();
  rect.Animate(start_time + AnimationCreator::usToDelta(10000));
  rect.transform_operations().Apply().TransformPoint(&p);
  EXPECT_VECTOR3DF_EQ(gfx::Vector3dF(20, 200, 2000), p);
}

}  // namespace vr
