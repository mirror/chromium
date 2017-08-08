// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/scroll/ScrollAnimatorCompositorCoordinator.h"

#include "platform/animation/CompositorAnimationTimeline.h"
#include "platform/graphics/test/FakeScrollableArea.h"
#include "platform/testing/RuntimeEnabledFeaturesTestHelpers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

using ScrollAnimatorCompositorCoordinatorTest = ::testing::Test;

class TestScrollAnimatorCompositorCoordinator
    : public ScrollAnimatorCompositorCoordinator {
 public:
  void SetScrollableArea(ScrollableArea* area) { area_ = area; }
  ScrollableArea* GetScrollableArea() const override { return area_; }
  void TickAnimation(double monotonic_time) override {}
  void NotifyCompositorAnimationFinished(int group_id) override {}
  void NotifyCompositorAnimationAborted(int group_id) override {}
  void LayerForCompositedScrollingDidChange(
      CompositorAnimationTimeline*) override {}

 private:
  ScrollableArea* area_;
};

TEST_F(ScrollAnimatorCompositorCoordinatorTest,
       ReattachCompositorPlayerIfNeededSPv2) {
  ScopedSlimmingPaintV2ForTest spv2(true);
  TestScrollAnimatorCompositorCoordinator coordinator;
  FakeScrollableArea area;
  coordinator.SetScrollableArea(&area);
  std::unique_ptr<CompositorAnimationTimeline> timeline =
      CompositorAnimationTimeline::Create();
  coordinator.ReattachCompositorPlayerIfNeeded(timeline);
}

TEST_F(ScrollAnimatorCompositorCoordinatorTest,
       UpdateImplOnlyCompositorAnimations) {}

}  // namespace blink
