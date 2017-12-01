// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_controller_observer_manager.h"

#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_scroll_end_animator.h"
#import "ios/chrome/browser/ui/fullscreen/test/fullscreen_model_test_util.h"
#import "ios/chrome/browser/ui/fullscreen/test/test_fullscreen_controller_observer.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test fixture for FullscreenControllerObserverManager.
class FullscreenControllerObserverManagerTest : public PlatformTest {
 public:
  FullscreenControllerObserverManagerTest()
      : PlatformTest(), observer_manager_(controller(), &model_) {
    SetUpFullscreenModelForTesting(&model_, 100);
    observer_manager_.AddObserver(&observer_);
  }
  ~FullscreenControllerObserverManagerTest() override {
    observer_manager_.RemoveObserver(&observer_);
    observer_manager_.Disconnect();
  }

  FullscreenController* controller() {
    // TestFullscreenControllerObserver doesn't use the FullscreenController
    // passed to its observer methods, so use a mocked pointer.
    static void* kFullscreenController = &kFullscreenController;
    return reinterpret_cast<FullscreenController*>(kFullscreenController);
  }
  FullscreenModel& model() { return model_; }
  TestFullscreenControllerObserver& observer() { return observer_; }

 private:
  FullscreenModel model_;
  FullscreenControllerObserverManager observer_manager_;
  TestFullscreenControllerObserver observer_;
};

// Tests that progress and scroll end animator are correctly forwarded to the
// observer.
TEST_F(FullscreenControllerObserverManagerTest, ObserveProgressAndScrollEnd) {
  SimulateFullscreenUserScrollForProgress(&model(), 0.5);
  EXPECT_EQ(observer().progress(), 0.5);
  FullscreenScrollEndAnimator* animator = observer().animator();
  EXPECT_TRUE(animator);
  EXPECT_EQ(animator.startProgress, 0.5);
  EXPECT_EQ(animator.finalProgress, 1.0);
}

// Tests that the enabled state is correctly forwarded to the observer.
TEST_F(FullscreenControllerObserverManagerTest, ObserveEnabledState) {
  EXPECT_TRUE(observer().enabled());
  model().IncrementDisabledCounter();
  EXPECT_FALSE(observer().enabled());
  model().DecrementDisabledCounter();
  EXPECT_TRUE(observer().enabled());
}
