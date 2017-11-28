// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_scroll_end_animation_controller.h"

#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_scroll_end_animator.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// TODO(crbug.com/778858): Remove ifdefs once iOS9 support is dropped.
#if defined(__IPHONE_10_0) && (__IPHONE_OS_VERSION_MIN_ALLOWED >= __IPHONE_10_0)
// Test fixture for FullscreenScrollEndAnimationController.
class FullscreenScrollEndAnimationControllerTest : public PlatformTest {
 public:
  FullscreenScrollEndAnimationControllerTest()
      : PlatformTest(), controller_(&model_) {}
  ~FullscreenScrollEndAnimationControllerTest() override {
    controller_.StopAnimating();
  }

  FullscreenModel& model() { return model_; }
  FullscreenScrollEndAnimationController& controller() { return controller_; }

 private:
  FullscreenModel model_;
  FullscreenScrollEndAnimationController controller_;
};

// Tests that CreateAnimator() creates scroll end animators as expected.
TEST_F(FullscreenScrollEndAnimationControllerTest, CreateAnimator) {
  // Create an animator that hides the toolbar.
  EXPECT_FALSE(controller().animator());
  controller().CreateAnimator(0.25);
  FullscreenScrollEndAnimator* animator = controller().animator();
  EXPECT_TRUE(animator);
  EXPECT_EQ(animator.startProgress, 0.25);
  EXPECT_EQ(animator.finalProgress, 0.0);
  controller().StopAnimating();
  // Create an animator that shows the toolbar.
  EXPECT_FALSE(controller().animator());
  controller().CreateAnimator(0.75);
  animator = controller().animator();
  EXPECT_TRUE(animator);
  EXPECT_EQ(animator.startProgress, 0.75);
  EXPECT_EQ(animator.finalProgress, 1.0);
}

// Tests that StopAnimating() resets the animator and updates the model with the
// final progress value.
TEST_F(FullscreenScrollEndAnimationControllerTest, StopAnimating) {
  EXPECT_FALSE(controller().animator());
  controller().CreateAnimator(0.25);
  EXPECT_TRUE(controller().animator());
  controller().StopAnimating();
  EXPECT_FALSE(controller().animator());
  EXPECT_EQ(model().progress(), 0.25);
}
#endif  // __IPHONE_10_0
