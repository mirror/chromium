// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Cocoa/Cocoa.h>

#include "base/debug/debugger.h"
#include "base/mac/scoped_nsobject.h"
#include "chrome/app/chrome_command_ids.h"
#import "chrome/browser/ui/cocoa/browser_window_controller.h"
#import "chrome/browser/ui/cocoa/framed_browser_window.h"
#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"

class FramedBrowserWindowTest : public CocoaTest {
 public:
  void SetUp() override {
    CocoaTest::SetUp();
    // Create a window.
    window_ = [[FramedBrowserWindow alloc]
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

  // Returns a canonical snapshot of the window.
  NSData* WindowContentsAsTIFF() {
    [window_ layoutIfNeeded];
    [window_ display];

    NSView* frameView = [window_ contentView];
    while ([frameView superview]) {
      frameView = [frameView superview];
    }

    // Inset to mask off left and right edges which vary in HighDPI.
    NSRect bounds = NSInsetRect([frameView bounds], 4, 0);

    // On 10.6, the grippy changes appearance slightly when painted the second
    // time in a textured window. Since this test cares about the window title,
    // cut off the bottom of the window.
    bounds.size.height -= 40;
    bounds.origin.y += 40;

    [frameView lockFocus];
    base::scoped_nsobject<NSBitmapImageRep> bitmap(
        [[NSBitmapImageRep alloc] initWithFocusedViewRect:bounds]);
    [frameView unlockFocus];

    return [bitmap TIFFRepresentation];
  }

  FramedBrowserWindow* window_;
};

// Baseline test that the window creates, displays, closes, and
// releases.
TEST_F(FramedBrowserWindowTest, ShowAndClose) {
  [window_ display];
}

// Test that undocumented title-hiding API we're using does the job.
TEST_F(FramedBrowserWindowTest, DoesHideTitle) {
  // The -display calls are not strictly necessary, but they do
  // make it easier to see what's happening when debugging (without
  // them the changes are never flushed to the screen).

  [window_ setTitle:@""];
  [window_ display];
  NSData* emptyTitleData = WindowContentsAsTIFF();

  [window_ setTitle:@"This is a title"];
  [window_ display];
  NSData* thisTitleData = WindowContentsAsTIFF();

  // The default window with a title should look different from the
  // window with an empty title.
  EXPECT_FALSE([emptyTitleData isEqualToData:thisTitleData]);

  [window_ setShouldHideTitle:YES];
  [window_ setTitle:@""];
  [window_ display];
  [window_ setTitle:@"This is a title"];
  [window_ display];
  NSData* hiddenTitleData = WindowContentsAsTIFF();

  // With our magic setting, the window with a title should look the
  // same as the window with an empty title.
  EXPECT_TRUE([window_ _isTitleHidden]);
  EXPECT_TRUE([emptyTitleData isEqualToData:hiddenTitleData]);
}
