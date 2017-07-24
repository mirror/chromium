// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/overlays/overlay_queue.h"

#import "base/mac/scoped_nsobject.h"
#include "base/memory/ptr_util.h"
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
    [parent_ addChildCoordinator:GetFirstOverlay()];
    [GetFirstOverlay() start];
    OverlayWasStarted();
  }
  void AddOverlay(BrowserCoordinator* overlay) {
    OverlayQueue::AddOverlay(overlay);
  }

 private:
  ParentCoordinator* parent_;
};

// Test BrowserCoordinator.
@interface TestBrowserCoordinator : BrowserCoordinator {
  OverlayQueue* _overlayQueue;
}
@property(nonatomic, readonly) BOOL cancelled;
@end

@implementation TestBrowserCoordinator
@synthesize cancelled = _cancelled;
- (void)stop {
  [super stop];
  [self overlayWasStopped];
}
- (void)cancelOverlay {
  _cancelled = YES;
}
@end

@implementation TestBrowserCoordinator (OverlaySupport)
- (BOOL)supportsOverlaying {
  return YES;
}
- (void)setOverlayQueue:(OverlayQueue*)overlayQueue {
  _overlayQueue = overlayQueue;
}
- (OverlayQueue*)overlayQueue {
  return _overlayQueue;
}
@end

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
  TestBrowserCoordinator* overlay = [[TestBrowserCoordinator alloc] init];
  queue().AddOverlay(overlay);
  EXPECT_TRUE(queue().HasQueuedOverlays());
  EXPECT_FALSE(queue().IsShowingOverlay());
  EXPECT_TRUE(observer().did_add_called());
}

// Tests that OverlayWasStopped() updates state properly.
TEST_F(OverlayQueueTest, OverlayWasStopped) {
  TestBrowserCoordinator* overlay = [[TestBrowserCoordinator alloc] init];
  queue().AddOverlay(overlay);
  queue().StartNextOverlay();
  queue().OverlayWasStopped(overlay);
  EXPECT_FALSE(queue().HasQueuedOverlays());
  EXPECT_FALSE(queue().IsShowingOverlay());
  EXPECT_TRUE(observer().stop_visible_called());
}

// Tests that the observer is notified when
TEST_F(OverlayQueueTest, ReplaceOverlay) {
  TestBrowserCoordinator* overlay = [[TestBrowserCoordinator alloc] init];
  queue().AddOverlay(overlay);
  queue().StartNextOverlay();
  TestBrowserCoordinator* replacement = [[TestBrowserCoordinator alloc] init];
  queue().ReplaceVisibleOverlay(replacement);
  EXPECT_TRUE(observer().will_replace_called());
  EXPECT_TRUE(observer().stop_visible_called());
}

// Tests that CancelOverlays() cancels all queued overlays for a WebState.
TEST_F(OverlayQueueTest, CancelOverlays) {
  TestBrowserCoordinator* overlay0 = [[TestBrowserCoordinator alloc] init];
  TestBrowserCoordinator* overlay1 = [[TestBrowserCoordinator alloc] init];
  queue().AddOverlay(overlay0);
  queue().AddOverlay(overlay1);
  queue().CancelOverlays();
  EXPECT_FALSE(queue().HasQueuedOverlays());
  EXPECT_FALSE(queue().IsShowingOverlay());
  EXPECT_TRUE(observer().did_cancel_called());
  EXPECT_TRUE(overlay0.cancelled);
  EXPECT_TRUE(overlay1.cancelled);
}
