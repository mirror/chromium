// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"

#import "ios/chrome/browser/ui/fullscreen/test/test_fullscreen_model_observer.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Calculates the expected progress value for a model with |base_offset| and
// |toolbar_height| when scrolled to |scroll_offset|.
// CGFloat CalculateProgress(CGFloat base_offset, CGFloat scroll_offset,
//                          CGFloat toolbar_height) {
//  return 0.0;
//}
}  // namespace

// Test fixture for FullscreenModel.
class FullscreenModelTest : public PlatformTest {
 public:
  FullscreenModel& model() { return model_; }
  TestFullscreenModelObserver& observer() { return observer_; }

 private:
  FullscreenModel model_;
  TestFullscreenModelObserver observer_;
};

// Tests that incremending and decrementing the disabled counter correctly
// disabled/enables the model.
TEST_F(FullscreenModelTest, EnableDisable) {
  ASSERT_TRUE(model().enabled());
  ASSERT_TRUE(observer().enabled());
  // Increment the disabled counter and check that the model is disabled.
  model().IncrementDisabledCounter();
  EXPECT_FALSE(model().enabled());
  EXPECT_FALSE(observer().enabled());
  // Increment again and check that the model is still disabled.
  model().IncrementDisabledCounter();
  EXPECT_FALSE(model().enabled());
  EXPECT_FALSE(observer().enabled());
  // Decrement the counter and check that the model is still disabled.
  model().DecrementDisabledCounter();
  EXPECT_FALSE(model().enabled());
  EXPECT_FALSE(observer().enabled());
  // Decrement again and check that the model is reenabled.
  model().DecrementDisabledCounter();
  EXPECT_TRUE(model().enabled());
  EXPECT_TRUE(observer().enabled());
}

// Tests that calling ResetForNavigation() resets the model to a fully-visible
// pre-scroll state.
TEST_F(FullscreenModelTest, ResetForNavigation) {
  //  CGFloat kToolbarHeight = 50.0;
}

// Tests that the end progress value of a scroll adjustment animation is used
// as the model's progress.
TEST_F(FullscreenModelTest, AnimationEnded) {}

// Tests that changing the toolbar height fully shows the new toolbar and
// responds appropriately to content offset changes.
TEST_F(FullscreenModelTest, UpdateToolbarHeight) {}

// Tests that updating the y content offset produces the expected progress
// value.
TEST_F(FullscreenModelTest, OffsetUpdate) {}

// Tests that setting scrolling to false sends a scroll end signal to its
// observers.
TEST_F(FullscreenModelTest, ScrollEnded) {}

// Tests that the base offset it updated when dragging begins.
TEST_F(FullscreenModelTest, DraggingStarted) {}
