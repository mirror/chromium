// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/browser_window_cocoa.h"

#include <memory>

#include "base/mac/mac_util.h"
#include "base/mac/scoped_nsobject.h"
#include "base/strings/string_util.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/ui/bookmarks/bookmark_utils.h"
#include "chrome/browser/ui/browser_window_observer.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#include "chrome/browser/ui/cocoa/test/cocoa_profile_test.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/notification_details.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#import "third_party/ocmock/gtest_support.h"

// Main test class.
class BrowserWindowCocoaTest : public CocoaProfileTest,
                               public BrowserWindowObserver {
 protected:
  // Returns whether OnShowStateChanged() has been called since the last time
  // this was checked.
  bool CheckShowStateChanged() {
    bool show_state_changed = show_state_changed_;
    // Reset the OnShowStateChanged tracker.
    show_state_changed_ = false;
    return show_state_changed;
  }

  BrowserWindowController* controller_ = nullptr;

 private:
  // CocoaProfileTest:
  void SetUp() override {
    CocoaProfileTest::SetUp();
    ASSERT_TRUE(browser());

    controller_ = [[BrowserWindowController alloc] initWithBrowser:browser()
                                                     takeOwnership:NO];
  }

  void TearDown() override {
    [[controller_ nsWindowController] close];
    CocoaProfileTest::TearDown();
  }

  // BrowserWindowObserver:
  void OnShowStateChanged() override { show_state_changed_ = true; }

  bool show_state_changed_ = false;
};

TEST_F(BrowserWindowCocoaTest, TestBookmarkBarVisible) {
  std::unique_ptr<BrowserWindowCocoa> bwc(
      new BrowserWindowCocoa(browser(), controller_));

  bool before = bwc->IsBookmarkBarVisible();
  chrome::ToggleBookmarkBarWhenVisible(profile());
  EXPECT_NE(before, bwc->IsBookmarkBarVisible());

  chrome::ToggleBookmarkBarWhenVisible(profile());
  EXPECT_EQ(before, bwc->IsBookmarkBarVisible());
}

TEST_F(BrowserWindowCocoaTest, TestWindowTitle) {
  std::unique_ptr<BrowserWindowCocoa> bwc(
      new BrowserWindowCocoa(browser(), controller_));
  NSString* playing_emoji = @"🔊";
  NSString* muting_emoji = @"🔇";
  EXPECT_EQ(static_cast<NSUInteger>(NSNotFound),
            [bwc->WindowTitle() rangeOfString:playing_emoji].location);
  EXPECT_EQ(static_cast<NSUInteger>(NSNotFound),
            [bwc->WindowTitle() rangeOfString:muting_emoji].location);
  bwc->UpdateAlertState(TabAlertState::AUDIO_PLAYING);
  EXPECT_NE(static_cast<NSUInteger>(NSNotFound),
            [bwc->WindowTitle() rangeOfString:playing_emoji].location);
  bwc->UpdateAlertState(TabAlertState::AUDIO_MUTING);
  EXPECT_NE(static_cast<NSUInteger>(NSNotFound),
            [bwc->WindowTitle() rangeOfString:muting_emoji].location);
  bwc->UpdateAlertState(TabAlertState::NONE);
  EXPECT_EQ(static_cast<NSUInteger>(NSNotFound),
            [bwc->WindowTitle() rangeOfString:playing_emoji].location);
  EXPECT_EQ(static_cast<NSUInteger>(NSNotFound),
            [bwc->WindowTitle() rangeOfString:muting_emoji].location);
}

// Test that IsMaximized() returns false when the browser window goes from
// maximized to minimized state - http://crbug/452976.
TEST_F(BrowserWindowCocoaTest, TestMinimizeState) {
  if (base::mac::IsOS10_10())
    return;  // Fails when swarmed. http://crbug.com/660582
  std::unique_ptr<BrowserWindowCocoa> bwc(
      new BrowserWindowCocoa(browser(), controller_));
  bwc->AddObserver(this);

  EXPECT_FALSE(bwc->IsMaximized());
  EXPECT_FALSE(bwc->IsMinimized());
  EXPECT_FALSE(CheckShowStateChanged());

  bwc->Maximize();
  EXPECT_TRUE(bwc->IsMaximized());
  EXPECT_FALSE(bwc->IsMinimized());
  EXPECT_TRUE(CheckShowStateChanged());

  bwc->Minimize();
  EXPECT_FALSE(bwc->IsMaximized());
  EXPECT_TRUE(bwc->IsMinimized());
  EXPECT_TRUE(CheckShowStateChanged());

  bwc->Restore();
  EXPECT_TRUE(bwc->IsMaximized());
  EXPECT_FALSE(bwc->IsMinimized());
  EXPECT_TRUE(CheckShowStateChanged());
}

// Tests that BrowserWindowCocoa::Close mimics the behavior of
// -[NSWindow performClose:].
class BrowserWindowCocoaCloseTest : public CocoaProfileTest {
 public:
  BrowserWindowCocoaCloseTest()
      : controller_(
            [OCMockObject mockForClass:[BrowserWindowController class]]),
        window_([OCMockObject mockForClass:[NSWindow class]]) {
    [[[controller_ stub] andReturn:nil] overlayWindow];
  }

  void CreateAndCloseBrowserWindow() {
    BrowserWindowCocoa browser_window(browser(), controller_);
    browser_window.Close();
  }

  id ValueYES() {
    BOOL v = YES;
    return OCMOCK_VALUE(v);
  }
  id ValueNO() {
    BOOL v = NO;
    return OCMOCK_VALUE(v);
  }

 protected:
  id controller_;
  id window_;
};

TEST_F(BrowserWindowCocoaCloseTest, DelegateRespondsYes) {
  [[[window_ stub] andReturn:controller_] delegate];
  [[[controller_ stub] andReturn:window_] window];
  [[[controller_ stub] andReturnValue:ValueYES()] windowShouldClose:window_];
  [[window_ expect] orderOut:nil];
  [[window_ expect] close];
  CreateAndCloseBrowserWindow();
  EXPECT_OCMOCK_VERIFY(controller_);
  EXPECT_OCMOCK_VERIFY(window_);
}

TEST_F(BrowserWindowCocoaCloseTest, DelegateRespondsNo) {
  [[[window_ stub] andReturn:controller_] delegate];
  [[[controller_ stub] andReturn:window_] window];
  [[[controller_ stub] andReturnValue:ValueNO()] windowShouldClose:window_];
  // Window should not be closed.
  CreateAndCloseBrowserWindow();
  EXPECT_OCMOCK_VERIFY(controller_);
  EXPECT_OCMOCK_VERIFY(window_);
}

// NSWindow does not implement |-windowShouldClose:|, but subclasses can
// implement it, and |-performClose:| will invoke it if implemented.
@interface BrowserWindowCocoaCloseWindow : NSWindow
- (BOOL)windowShouldClose:(id)window;
@end
@implementation BrowserWindowCocoaCloseWindow
- (BOOL)windowShouldClose:(id)window {
  return YES;
}
@end

TEST_F(BrowserWindowCocoaCloseTest, WindowRespondsYes) {
  window_ = [OCMockObject mockForClass:[BrowserWindowCocoaCloseWindow class]];
  [[[window_ stub] andReturn:nil] delegate];
  [[[controller_ stub] andReturn:window_] window];
  [[[window_ stub] andReturnValue:ValueYES()] windowShouldClose:window_];
  [[window_ expect] orderOut:nil];
  [[window_ expect] close];
  CreateAndCloseBrowserWindow();
  EXPECT_OCMOCK_VERIFY(controller_);
  EXPECT_OCMOCK_VERIFY(window_);
}

TEST_F(BrowserWindowCocoaCloseTest, WindowRespondsNo) {
  window_ = [OCMockObject mockForClass:[BrowserWindowCocoaCloseWindow class]];
  [[[window_ stub] andReturn:nil] delegate];
  [[[controller_ stub] andReturn:window_] window];
  [[[window_ stub] andReturnValue:ValueNO()] windowShouldClose:window_];
  // Window should not be closed.
  CreateAndCloseBrowserWindow();
  EXPECT_OCMOCK_VERIFY(controller_);
  EXPECT_OCMOCK_VERIFY(window_);
}

TEST_F(BrowserWindowCocoaCloseTest, DelegateRespondsYesWindowRespondsNo) {
  window_ = [OCMockObject mockForClass:[BrowserWindowCocoaCloseWindow class]];
  [[[window_ stub] andReturn:controller_] delegate];
  [[[controller_ stub] andReturn:window_] window];
  [[[controller_ stub] andReturnValue:ValueYES()] windowShouldClose:window_];
  [[[window_ stub] andReturnValue:ValueNO()] windowShouldClose:window_];
  [[window_ expect] orderOut:nil];
  [[window_ expect] close];
  CreateAndCloseBrowserWindow();
  EXPECT_OCMOCK_VERIFY(controller_);
  EXPECT_OCMOCK_VERIFY(window_);
}

TEST_F(BrowserWindowCocoaCloseTest, DelegateRespondsNoWindowRespondsYes) {
  window_ = [OCMockObject mockForClass:[BrowserWindowCocoaCloseWindow class]];
  [[[window_ stub] andReturn:controller_] delegate];
  [[[controller_ stub] andReturn:window_] window];
  [[[controller_ stub] andReturnValue:ValueNO()] windowShouldClose:window_];
  [[[window_ stub] andReturnValue:ValueYES()] windowShouldClose:window_];
  // Window should not be closed.
  CreateAndCloseBrowserWindow();
  EXPECT_OCMOCK_VERIFY(controller_);
  EXPECT_OCMOCK_VERIFY(window_);
}

TEST_F(BrowserWindowCocoaCloseTest, NoResponseFromDelegateNorWindow) {
  [[[window_ stub] andReturn:nil] delegate];
  [[[controller_ stub] andReturn:window_] window];
  [[window_ expect] orderOut:nil];
  [[window_ expect] close];
  CreateAndCloseBrowserWindow();
  EXPECT_OCMOCK_VERIFY(controller_);
  EXPECT_OCMOCK_VERIFY(window_);
}

// TODO(???): test other methods of BrowserWindowCocoa
