// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/main_content/main_content_ui_forwarder.h"

#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"
#import "ios/chrome/browser/ui/main_content/main_content_ui_forwarder.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - MainContentUIObserver

// An object that observes the main content scroll view's y offset, scrolling,
// and dragging state.
@interface MainContentUIObserver : NSObject<ChromeBroadcastObserver>

// The broadcaster.  Setting will start observing the new value.
@property(nonatomic, strong) ChromeBroadcaster* broadcaster;
// The broadcasted UI state observed by this object.
@property(nonatomic, readonly) CGFloat yOffset;
@property(nonatomic, readonly, getter=isScrolling) BOOL scrolling;
@property(nonatomic, readonly, getter=isDragging) BOOL dragging;

@end

@implementation MainContentUIObserver
@synthesize broadcaster = _broadcaster;
@synthesize yOffset = _yOffset;
@synthesize scrolling = _scrolling;
@synthesize dragging = _dragging;

- (void)setBroadcaster:(ChromeBroadcaster*)broadcaster {
  [_broadcaster removeObserver:self
                   forSelector:@selector(broadcastContentScrollOffset:)];
  [_broadcaster removeObserver:self
                   forSelector:@selector(broadcastScrollViewIsScrolling:)];
  [_broadcaster removeObserver:self
                   forSelector:@selector(broadcastScrollViewIsDragging:)];
  _broadcaster = broadcaster;
  [_broadcaster addObserver:self
                forSelector:@selector(broadcastContentScrollOffset:)];
  [_broadcaster addObserver:self
                forSelector:@selector(broadcastScrollViewIsScrolling:)];
  [_broadcaster addObserver:self
                forSelector:@selector(broadcastScrollViewIsDragging:)];
}

- (void)broadcastContentScrollOffset:(CGFloat)offset {
  _yOffset = offset;
}

- (void)broadcastScrollViewIsScrolling:(BOOL)scrolling {
  _scrolling = scrolling;
}

- (void)broadcastScrollViewIsDragging:(BOOL)dragging {
  _dragging = dragging;
}

@end

#pragma mark - MainContentUIForwarderTest

// Test fixture for MainContentUIForarders.
class MainContentUIForwarderTest : public PlatformTest {
 public:
  MainContentUIForwarderTest()
      : PlatformTest(),
        forwarder_([[MainContentUIForwarder alloc] init]),
        broadcaster_([[ChromeBroadcaster alloc] init]),
        observer_([[MainContentUIObserver alloc] init]) {
    // Begin observing broadcast values from |broadcaster_|.
    observer_.broadcaster = broadcaster_;
  }

  ~MainContentUIForwarderTest() override {
    // Stop observing broadcast values.
    observer_.broadcaster = nil;
  }

  // The forwarder being tested.
  MainContentUIForwarder* forwarder() { return forwarder_; }
  // The broadcaster used by the forwarder.
  ChromeBroadcaster* broadcaster() { return broadcaster_; }
  // The observer for the UI state being forwarded.
  MainContentUIObserver* observer() { return observer_; }

 private:
  __strong MainContentUIForwarder* forwarder_;
  __strong ChromeBroadcaster* broadcaster_;
  __strong MainContentUIObserver* observer_;
};

// Tests that |-startBroadcasting:| and |-stopBroadcasting:| correctly update
// |broadcasting|.
TEST_F(MainContentUIForwarderTest, StartStopBroadcasting) {
  EXPECT_FALSE(forwarder().broadcasting);
  [forwarder() startBroadcasting:broadcaster()];
  EXPECT_TRUE(forwarder().broadcasting);
  [forwarder() stopBroadcasting:broadcaster()];
  EXPECT_FALSE(forwarder().broadcasting);
}

// Tests that the y content offset is correctly broadcast as the result of
// |-scrollViewDidScrollToOffset:|.
TEST_F(MainContentUIForwarderTest, BroadcastOffset) {
  const CGFloat kYOffset = 150.0;
  ASSERT_TRUE(AreCGFloatsEqual(0.0, observer().yOffset));
  [forwarder() startBroadcasting:broadcaster()];
  [forwarder() scrollViewDidScrollToOffset:CGPointMake(0.0, kYOffset)];
  EXPECT_TRUE(AreCGFloatsEqual(kYOffset, observer().yOffset));
}

// Tests broadcasting drag and scroll events.
TEST_F(MainContentUIForwarderTest, BroadcastScrollingAndDragging) {
  [forwarder() startBroadcasting:broadcaster()];
  ASSERT_FALSE(observer().scrolling);
  ASSERT_FALSE(observer().dragging);
  [forwarder() scrollViewWillBeginDragging:nil];
  EXPECT_TRUE(observer().scrolling);
  EXPECT_TRUE(observer().dragging);
  [forwarder() scrollViewDidEndDragging:nil residualVelocity:CGPointMake(0, 5)];
  EXPECT_TRUE(observer().scrolling);
  EXPECT_FALSE(observer().dragging);
  [forwarder() scrollViewDidEndDecelerating];
  EXPECT_FALSE(observer().scrolling);
  EXPECT_FALSE(observer().dragging);
}
