// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <AppKit/AppKit.h>

#import "base/mac/mac_util.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/tabbed_browser_window.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "third_party/ocmock/OCMock/OCMock.h"

namespace {
NSString* const kAppleTextDirectionDefaultsKey = @"AppleTextDirection";
NSString* const kForceRTLWritingDirectionDefaultsKey =
    @"NSForceRightToLeftWritingDirection";
constexpr CGFloat kWindowButtonInset = 11;
}  // namespace

class TabbedBrowserWindowTest : public CocoaTest {
 protected:
  TabbedBrowserWindowTest() = default;

  void SetUp() override {
    CocoaTest::SetUp();
    window_ = [[TabbedBrowserWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 800, 600)];
    [window() orderBack:nil];
  }

  void TearDown() override {
    [window() close];
    CocoaTest::TearDown();
  }

  TabbedBrowserWindow* window() { return window_; };

 private:
  TabbedBrowserWindow* window_;

  DISALLOW_COPY_AND_ASSIGN(TabbedBrowserWindowTest);
};

// Test to make sure that our window widgets are in the right place.
TEST_F(TabbedBrowserWindowTest, WindowWidgetLocation) {
  id controller = [OCMockObject mockForClass:[BrowserWindowController class]];
  [[[controller stub] andReturnValue:@YES]
      isKindOfClass:[BrowserWindowController class]];
  [[[controller expect] andReturnValue:@NO] hasTitleBar];
  [[[controller expect] andReturnValue:@YES] isTabbedWindow];
  [window() setWindowController:controller];

  NSView* closeBoxControl = [window() standardWindowButton:NSWindowCloseButton];
  EXPECT_TRUE(closeBoxControl);
  NSRect closeBoxFrame =
      [closeBoxControl convertRect:[closeBoxControl bounds] toView:nil];
  NSRect windowBounds = [window() frame];
  windowBounds = [[window() contentView] convertRect:windowBounds fromView:nil];
  windowBounds.origin = NSZeroPoint;
  EXPECT_EQ(NSMaxY(closeBoxFrame), NSMaxY(windowBounds) - kWindowButtonInset);
  EXPECT_EQ(NSMinX(closeBoxFrame), kWindowButtonInset);
  [window() setWindowController:nil];
}

class TabbedBrowserWindowRTLTest : public TabbedBrowserWindowTest {
 public:
  TabbedBrowserWindowRTLTest() = default;

  void SetUp() override {
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    originalAppleTextDirection_ =
        [defaults boolForKey:kAppleTextDirectionDefaultsKey];
    originalRTLWritingDirection_ =
        [defaults boolForKey:kForceRTLWritingDirectionDefaultsKey];
    [defaults setBool:YES forKey:kAppleTextDirectionDefaultsKey];
    [defaults setBool:YES forKey:kForceRTLWritingDirectionDefaultsKey];
    TabbedBrowserWindowTest::SetUp();
  }

  void TearDown() override {
    TabbedBrowserWindowTest::TearDown();
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    [defaults setBool:originalAppleTextDirection_
               forKey:kAppleTextDirectionDefaultsKey];
    [defaults setBool:originalRTLWritingDirection_
               forKey:kForceRTLWritingDirectionDefaultsKey];
  }

 private:
  BOOL originalAppleTextDirection_;
  BOOL originalRTLWritingDirection_;

  DISALLOW_COPY_AND_ASSIGN(TabbedBrowserWindowRTLTest);
};

// Test to make sure that our window widgets are in the right place.
// Currently, this is the same exact test as above, since RTL button
// layout is behind the ExperimentalMacRTL flag. However, this ensures
// that our calculations are correct for Sierra RTL, which lays out
// the window buttons in reverse by default. See crbug/662079.
TEST_F(TabbedBrowserWindowRTLTest, WindowWidgetLocation) {
  id controller = [OCMockObject mockForClass:[BrowserWindowController class]];
  [[[controller stub] andReturnValue:@YES]
      isKindOfClass:[BrowserWindowController class]];
  [[[controller expect] andReturnValue:@NO] hasTitleBar];
  [[[controller expect] andReturnValue:@YES] isTabbedWindow];
  [window() setWindowController:controller];

  NSView* closeBoxControl = [window() standardWindowButton:NSWindowCloseButton];
  EXPECT_TRUE(closeBoxControl);
  NSRect closeBoxFrame =
      [closeBoxControl convertRect:[closeBoxControl bounds] toView:nil];
  NSRect windowBounds = [window() frame];
  windowBounds = [[window() contentView] convertRect:windowBounds fromView:nil];
  windowBounds.origin = NSZeroPoint;
  EXPECT_EQ(NSMaxY(closeBoxFrame), NSMaxY(windowBounds) - kWindowButtonInset);
  EXPECT_EQ(NSMinX(closeBoxFrame), kWindowButtonInset);
  [window() setWindowController:nil];
}

@interface NSThemeFrame : NSView
@end

// Test that NSThemeFrame and NSWindow respond to the undocumented methods
// which TabbedBrowserWindowFrame and TabbedBrowserWindow override.
TEST(TabbedBrowserWindowOverrideTest, UndocumentedMethods) {
  EXPECT_TRUE([NSThemeFrame
      instancesRespondToSelector:@selector(_minXTitlebarWidgetInset)]);
  EXPECT_TRUE([NSThemeFrame
      instancesRespondToSelector:@selector(_minYTitlebarButtonsOffset)]);
  EXPECT_TRUE(
      [NSThemeFrame instancesRespondToSelector:@selector(_titlebarHeight)]);
  EXPECT_TRUE([NSThemeFrame instancesRespondToSelector:@selector
                            (contentRectForFrameRect:styleMask:)]);
  EXPECT_TRUE([NSThemeFrame instancesRespondToSelector:@selector
                            (frameRectForContentRect:styleMask:)]);
  if (base::mac::IsAtLeastOS10_12()) {
    EXPECT_TRUE([NSThemeFrame
        instancesRespondToSelector:@selector(_shouldFlipTrafficLightsForRTL)]);
  } else if (!base::mac::IsAtLeastOS10_10()) {
    EXPECT_TRUE([NSThemeFrame
        instancesRespondToSelector:@selector(_fullScreenButtonOrigin)]);
    EXPECT_TRUE(
        [NSThemeFrame instancesRespondToSelector:@selector(_setContentView:)]);
  }

  EXPECT_TRUE(
      [NSWindow respondsToSelector:@selector(frameViewClassForStyleMask:)]);
}
