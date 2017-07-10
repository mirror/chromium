// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <AppKit/AppKit.h>

#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/tabbed_browser_window.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"

namespace {
NSString* const kAppleTextDirectionDefaultsKey = @"AppleTextDirection";
NSString* const kForceRTLWritingDirectionDefaultsKey =
    @"NSForceRightToLeftWritingDirection";
const CGFloat kWindowButtonInset = 11;
}  // namespace

class TabbedBrowserWindowTest : public CocoaTest {
 public:
  void SetUp() override {
    CocoaTest::SetUp();
    // Create a window.
    window_ = [[TabbedBrowserWindow alloc]
        initWithContentRect:NSMakeRect(0, 0, 800, 600)];
    if (base::debug::BeingDebugged()) {
      [window_ orderFront:nil];
    } else {
      [window_ orderBack:nil];
    }
  }

  void TearDown() override {
    [window_ close];
    CocoaTest::TearDown();
  }

  TabbedBrowserWindow* window_;
};

// Test to make sure that our window widgets are in the right place.
TEST_F(TabbedBrowserWindowTest, WindowWidgetLocation) {
  // Update window layout according to existing layout constraints.
  [window_ layoutIfNeeded];
  id controller = [OCMockObject mockForClass:[BrowserWindowController class]];
  [[[controller stub] andReturnValue:@YES]
      isKindOfClass:[BrowserWindowController class]];
  [[[controller expect] andReturnValue:@NO] hasTitleBar];
  [[[controller expect] andReturnValue:@YES] isTabbedWindow];
  [window_ setWindowController:controller];

  NSView* closeBoxControl = [window_ standardWindowButton:NSWindowCloseButton];
  EXPECT_TRUE(closeBoxControl);
  NSRect closeBoxFrame =
      [closeBoxControl convertRect:[closeBoxControl bounds] toView:nil];
  NSRect windowBounds = [window_ frame];
  windowBounds = [[window_ contentView] convertRect:windowBounds fromView:nil];
  windowBounds.origin = NSZeroPoint;
  EXPECT_EQ(NSMaxY(closeBoxFrame), NSMaxY(windowBounds) - kWindowButtonInset);
  EXPECT_EQ(NSMinX(closeBoxFrame), kWindowButtonInset);
  [window_ setWindowController:nil];
}

class TabbedBrowserWindowRTLTest : public TabbedBrowserWindowTest {
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
};

// Test to make sure that our window widgets are in the right place.
// Currently, this is the same exact test as above, since RTL button
// layout is behind the ExperimentalMacRTL flag. However, this ensures
// that our calculations are correct for Sierra RTL, which lays out
// the window buttons in reverse by default. See crbug/662079.
TEST_F(TabbedBrowserWindowRTLTest, WindowWidgetLocation) {
  [window_ close];
  window_ = [[TabbedBrowserWindow alloc]
      initWithContentRect:NSMakeRect(0, 0, 800, 600)];
  // Update window layout according to existing layout constraints.
  [window_ layoutIfNeeded];
  id controller = [OCMockObject mockForClass:[BrowserWindowController class]];
  [[[controller stub] andReturnValue:@YES]
      isKindOfClass:[BrowserWindowController class]];
  [[[controller expect] andReturnValue:@NO] hasTitleBar];
  [[[controller expect] andReturnValue:@YES] isTabbedWindow];
  [window_ setWindowController:controller];

  NSView* closeBoxControl = [window_ standardWindowButton:NSWindowCloseButton];
  EXPECT_TRUE(closeBoxControl);
  NSRect closeBoxFrame =
      [closeBoxControl convertRect:[closeBoxControl bounds] toView:nil];
  NSRect windowBounds = [window_ frame];
  windowBounds = [[window_ contentView] convertRect:windowBounds fromView:nil];
  windowBounds.origin = NSZeroPoint;
  EXPECT_EQ(NSMaxY(closeBoxFrame), NSMaxY(windowBounds) - kWindowButtonInset);
  EXPECT_EQ(NSMinX(closeBoxFrame), kWindowButtonInset);
  [window_ setWindowController:nil];
}
