// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/fullscreen/fullscreen_broadcast_forwarder.h"

#import "ios/chrome/browser/ui/broadcaster/chrome_broadcaster.h"
#import "ios/chrome/browser/ui/fullscreen/fullscreen_model.h"
#import "ios/chrome/browser/ui/fullscreen/test/test_fullscreen_broadcast_receiver.h"
#include "ios/chrome/browser/ui/ui_util.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

#pragma mark - TestFullscreenUIBroadcaster

// Object to use in test to send broadcast values through the
// FullscreenBroacastForwarder.
@interface TestUIBroadcaster : NSObject
// The broadcaster to use.
@property(nonatomic, weak) ChromeBroadcaster* broadcaster;
// The broadcasted properties.
@property(nonatomic) CGFloat scrollOffset;
@property(nonatomic, getter=isDragging) BOOL dragging;
@property(nonatomic, getter=isScrolling) BOOL scrolling;
@property(nonatomic) CGFloat toolbarHeight;
@end

@implementation TestUIBroadcaster
@synthesize broadcaster = _broadcaster;
@synthesize scrollOffset = _scrollOffset;
@synthesize dragging = _dragging;
@synthesize scrolling = _scrolling;
@synthesize toolbarHeight = _toolbarHeight;

- (void)setBroadcaster:(ChromeBroadcaster*)broadcaster {
  if (_broadcaster == broadcaster)
    return;
  [_broadcaster
      stopBroadcastingForSelector:@selector(broadcastContentScrollOffset:)];
  [_broadcaster
      stopBroadcastingForSelector:@selector(broadcastScrollViewIsScrolling:)];
  [_broadcaster
      stopBroadcastingForSelector:@selector(broadcastScrollViewIsDragging:)];
  [_broadcaster stopBroadcastingForSelector:@selector(broadcastToolbarHeight:)];
  _broadcaster = broadcaster;
  [_broadcaster broadcastValue:@"scrollOffset"
                      ofObject:self
                      selector:@selector(broadcastContentScrollOffset:)];
  [_broadcaster broadcastValue:@"scrolling"
                      ofObject:self
                      selector:@selector(broadcastScrollViewIsScrolling:)];
  [_broadcaster broadcastValue:@"dragging"
                      ofObject:self
                      selector:@selector(broadcastScrollViewIsDragging:)];
  [_broadcaster broadcastValue:@"toolbarHeight"
                      ofObject:self
                      selector:@selector(broadcastToolbarHeight:)];
}

@end

#pragma mark - FullscreenBroadcastForwarderTest

// Test fixture for FullscreenBroadcastForwarder.
class FullscreenBroadcastForwarderTest : public PlatformTest {
 public:
  FullscreenBroadcastForwarderTest()
      : PlatformTest(),
        receiver_([[TestFullscreenBroadcastReceiver alloc] init]),
        broadcaster_([[ChromeBroadcaster alloc] init]),
        forwarder_([[FullscreenBroadcastForwarder alloc]
            initWithBroadcaster:broadcaster_
                       receiver:receiver_]),
        ui_broadcaster_([[TestUIBroadcaster alloc] init]) {
    ui_broadcaster_.broadcaster = broadcaster_;
  }

  ~FullscreenBroadcastForwarderTest() override {
    ui_broadcaster_.broadcaster = nil;
  }

  TestUIBroadcaster* ui_broadcaster() { return ui_broadcaster_; }
  TestFullscreenBroadcastReceiver* receiver() { return receiver_; }

 private:
  __strong TestFullscreenBroadcastReceiver* receiver_;
  __strong ChromeBroadcaster* broadcaster_;
  __strong FullscreenBroadcastForwarder* forwarder_;
  __strong TestUIBroadcaster* ui_broadcaster_;
};

// Tests that y scroll offsets are correctly forwarded to the receiver.
TEST_F(FullscreenBroadcastForwarderTest, ScrollOffset) {
  const CGFloat kScrollOffset = 100.0;
  ui_broadcaster().scrollOffset = kScrollOffset;
  EXPECT_TRUE(AreCGFloatsEqual(receiver().scrollOffset, kScrollOffset));
}

// Tests that dragging is correctly forwarded to the receiver.
TEST_F(FullscreenBroadcastForwarderTest, Dragging) {
  ASSERT_FALSE(receiver().dragging);
  ui_broadcaster().dragging = YES;
  EXPECT_TRUE(receiver().dragging);
  ui_broadcaster().dragging = NO;
  EXPECT_FALSE(receiver().dragging);
}

// Tests that scrolling is correctly forwarded to the receiver.
TEST_F(FullscreenBroadcastForwarderTest, Scrolling) {
  ASSERT_FALSE(receiver().scrolling);
  ui_broadcaster().scrolling = YES;
  EXPECT_TRUE(receiver().scrolling);
  ui_broadcaster().scrolling = NO;
  EXPECT_FALSE(receiver().scrolling);
}

// Tests that the toolbar height correctly forwarded to the receiver.
TEST_F(FullscreenBroadcastForwarderTest, ToolbarHeight) {
  const CGFloat kToolbarHeight = 100.0;
  ui_broadcaster().toolbarHeight = kToolbarHeight;
  EXPECT_TRUE(AreCGFloatsEqual(receiver().toolbarHeight, kToolbarHeight));
}
