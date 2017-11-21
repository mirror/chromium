// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/broadcaster/chrome_broadcast_observer_bridge.h"

#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test implementation of ChromeBroadcastObserverInterface.
class TestChromeBroadcastObserver : public ChromeBroadcastObserverInterface {
 public:
  // Received broadcast values.
  bool tab_strip_visible() const { return tab_strip_visible_; }
  CGFloat scroll_offset() const { return scroll_offset_; }
  bool scroll_view_scrolling() const { return scroll_view_scrolling_; }
  bool scroll_view_dragging() const { return scroll_view_dragging_; }
  ntp_home::PanelIdentifier ntp_panel() const { return ntp_panel_; }
  CGRect omnibox_frame() const { return omnibox_frame_; }

 private:
  // ChromeBroadcastObserverInterface:
  void OnTabStripVisbibleBroadcasted(bool visible) override {
    tab_strip_visible_ = visible;
  }
  void OnContentScrollOffsetBroadcasted(CGFloat offset) override {
    scroll_offset_ = offset;
  }
  void OnScrollViewIsScrollingBroadcasted(bool scrolling) override {
    scroll_view_scrolling_ = scrolling;
  }
  void OnScrollViewIsDraggingBroadcasted(bool dragging) override {
    scroll_view_dragging_ = dragging;
  }
  void OnSelectedNTPPanelBroadcasted(
      ntp_home::PanelIdentifier panelIdentifier) override {
    ntp_panel_ = panelIdentifier;
  }
  void OnOmniboxFrameBroadcasted(CGRect frame) override {
    omnibox_frame_ = frame;
  }

  bool tab_strip_visible_ = false;
  CGFloat scroll_offset_ = 0.0;
  bool scroll_view_scrolling_ = false;
  bool scroll_view_dragging_ = false;
  ntp_home::PanelIdentifier ntp_panel_ = ntp_home::NONE;
  CGRect omnibox_frame_ = CGRectZero;
};

// Test fixture for ChromeBroadcastOberverBridge.
class ChromeBroadcastObserverTest : public PlatformTest {
 public:
  ChromeBroadcastObserverTest()
      : PlatformTest(),
        bridge_([[ChromeBroadcastOberverBridge alloc]
            initWithObserver:&observer_]) {}

  const TestChromeBroadcastObserver& observer() { return observer_; }
  id<ChromeBroadcastObserver> bridge() { return bridge_; }

 private:
  TestChromeBroadcastObserver observer_;
  __strong ChromeBroadcastOberverBridge* bridge_ = nil;
};

// Tests that |-broadcastTabStripVisible:| is correctly forwarded to the
// observer.
TEST_F(ChromeBroadcastObserverTest, TabStripVisible) {
  ASSERT_FALSE(observer().tab_strip_visible());
  [bridge() broadcastTabStripVisible:YES];
  EXPECT_TRUE(observer().tab_strip_visible());
}

// Tests that |-broadcastContentScrollOffset:| is correctly forwarded to the
// observer.
TEST_F(ChromeBroadcastObserverTest, ContentOffset) {
  ASSERT_EQ(observer().scroll_offset(), 0.0);
  const CGFloat kOffset = 50.0;
  [bridge() broadcastContentScrollOffset:kOffset];
  EXPECT_EQ(observer().scroll_offset(), kOffset);
}

// Tests that |-broadcastScrollViewIsScrolling:| is correctly forwarded to the
// observer.
TEST_F(ChromeBroadcastObserverTest, ScrollViewIsScrolling) {
  ASSERT_FALSE(observer().scroll_view_scrolling());
  [bridge() broadcastScrollViewIsScrolling:YES];
  EXPECT_TRUE(observer().scroll_view_scrolling());
}

// Tests that |-broadcastScrollViewIsDragging:| is correctly forwarded to the
// observer.
TEST_F(ChromeBroadcastObserverTest, ScrollViewIsDragging) {
  ASSERT_FALSE(observer().scroll_view_dragging());
  [bridge() broadcastScrollViewIsDragging:YES];
  EXPECT_TRUE(observer().scroll_view_dragging());
}

// Tests that |-broadcastSelectedNTPPanel:| is correctly forwarded to the
// observer.
TEST_F(ChromeBroadcastObserverTest, NTPPanel) {
  ASSERT_EQ(observer().ntp_panel(), ntp_home::NONE);
  const ntp_home::PanelIdentifier kPanel = ntp_home::HOME_PANEL;
  [bridge() broadcastSelectedNTPPanel:kPanel];
  EXPECT_EQ(observer().ntp_panel(), kPanel);
}

// Tests that |-broadcastOmniboxFrame:| is correctly forwarded to the observer.
TEST_F(ChromeBroadcastObserverTest, OmniboxFrame) {
  EXPECT_TRUE(CGRectEqualToRect(observer().omnibox_frame(), CGRectZero));
  const CGRect kFrame = CGRectMake(0, 1, 2, 3);
  [bridge() broadcastOmniboxFrame:kFrame];
  EXPECT_TRUE(CGRectEqualToRect(observer().omnibox_frame(), kFrame));
}
