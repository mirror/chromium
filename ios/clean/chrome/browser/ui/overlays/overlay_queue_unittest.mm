// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/overlays/overlay_queue.h"

#import "base/mac/scoped_nsobject.h"
#include "base/memory/ptr_util.h"
#import "ios/clean/chrome/browser/ui/overlays/overlay_coordinator.h"
#import "ios/clean/chrome/browser/ui/overlays/overlay_queue_observer.h"
#import "ios/shared/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// BrowserCoordinator that can be used as the parent for overlays.
@interface ParentCoordinator : BrowserCoordinator
@end

@implementation ParentCoordinator
- (UIViewController*)viewController {
  return nil;
}
@end

// Test OverlayQueue implementation.  Subclass-specific functionality will be
// tested in other files.
class TestOverlayQueue : public OverlayQueue {
 public:
  TestOverlayQueue() : parent_([[ParentCoordinator alloc] init]) {}
  void StartNextOverlay() override {
    [GetFirstOverlay() startOverlayingCoordinator:parent_];
    OverlayWasStarted();
  }
  void AddOverlay(OverlayCoordinator* overlay) {
    OverlayQueue::AddOverlay(overlay);
  }

 private:
  ParentCoordinator* parent_;
};

// Test OverlayQueueObserver.
class TestOverlayQueueObserver : public OverlayQueueObserver {
 public:
  TestOverlayQueueObserver()
      : did_add_called_(false),
        will_replace_called_(false),
        stop_visible_called_(false),
        did_cancel_called_(false) {}

  void OverlayQueueDidAddOverlay(OverlayQueue* queue) override {
    did_add_called_ = true;
  }
  void OverlayQueueWillReplaceVisibleOverlay(OverlayQueue* queue) override {
    will_replace_called_ = true;
  }
  void OverlayQueueDidStopVisibleOverlay(OverlayQueue* queue) override {
    stop_visible_called_ = true;
  }
  void OverlayQueueDidCancelOverlays(OverlayQueue* queue) override {
    did_cancel_called_ = true;
  }

  bool did_add_called() { return did_add_called_; }
  bool will_replace_called() { return will_replace_called_; }
  bool stop_visible_called() { return stop_visible_called_; }
  bool did_cancel_called() { return did_cancel_called_; }

 private:
  bool did_add_called_;
  bool will_replace_called_;
  bool stop_visible_called_;
  bool did_cancel_called_;
};

// Test OverlayCoordinator.
@interface TestOverlayCoordinator : OverlayCoordinator
@property(nonatomic, readonly, getter=isCancelled) BOOL cancelled;
@end

@implementation TestOverlayCoordinator
@synthesize cancelled = _cancelled;
- (void)cancel {
  _cancelled = YES;
}
@end

class OverlayQueueTest : public PlatformTest {
 public:
  OverlayQueueTest() : PlatformTest() { queue_.AddObserver(&observer_); }

  TestOverlayQueue& queue() { return queue_; }
  TestOverlayQueueObserver& observer() { return observer_; }

 private:
  TestOverlayQueue queue_;
  TestOverlayQueueObserver observer_;
};

// Tests that adding an overlay to the queue updates state accordingly and
// notifies observers.
TEST_F(OverlayQueueTest, AddOverlay) {
  OverlayCoordinator* overlay = [[OverlayCoordinator alloc] init];
  queue().AddOverlay(overlay);
  EXPECT_TRUE(queue().HasQueuedOverlays());
  EXPECT_FALSE(queue().IsShowingOverlay());
  EXPECT_TRUE(observer().did_add_called());
}

// Tests that OverlayWasStopped() updates state properly.
TEST_F(OverlayQueueTest, OverlayWasStopped) {
  OverlayCoordinator* overlay = [[OverlayCoordinator alloc] init];
  queue().AddOverlay(overlay);
  queue().StartNextOverlay();
  queue().OverlayWasStopped(overlay);
  EXPECT_FALSE(queue().HasQueuedOverlays());
  EXPECT_FALSE(queue().IsShowingOverlay());
  EXPECT_TRUE(observer().stop_visible_called());
}

// Tests that the observer is notified when
TEST_F(OverlayQueueTest, ReplaceOverlay) {
  OverlayCoordinator* overlay = [[OverlayCoordinator alloc] init];
  queue().AddOverlay(overlay);
  queue().StartNextOverlay();
  OverlayCoordinator* replacement = [[OverlayCoordinator alloc] init];
  queue().ReplaceVisibleOverlay(replacement);
  EXPECT_TRUE(observer().will_replace_called());
  EXPECT_TRUE(observer().stop_visible_called());
}

// Tests that CancelOverlays() cancels all queued overlays for a WebState.
TEST_F(OverlayQueueTest, CancelOverlays) {
  TestOverlayCoordinator* overlay0 = [[TestOverlayCoordinator alloc] init];
  TestOverlayCoordinator* overlay1 = [[TestOverlayCoordinator alloc] init];
  queue().AddOverlay(overlay0);
  queue().AddOverlay(overlay1);
  queue().CancelOverlays();
  EXPECT_FALSE(queue().HasQueuedOverlays());
  EXPECT_FALSE(queue().IsShowingOverlay());
  EXPECT_TRUE(observer().did_cancel_called());
  EXPECT_TRUE(overlay0.cancelled);
  EXPECT_TRUE(overlay1.cancelled);
}
