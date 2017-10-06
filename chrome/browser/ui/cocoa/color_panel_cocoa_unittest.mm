// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/color_chooser_mac.h"

#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

@interface NSColorPanel (Private)
// Private method returning the NSColorPanel's target.
- (id)__target;
@end

namespace {

class ColorPanelCocoaTest : public CocoaTest {};

// Test default values; make sure they are what they should be.
TEST_F(ColorPanelCocoaTest, Dealloc) {
  ColorChooserMac color_chooser_mac(nullptr, SK_ColorBLACK);

  ColorPanelCocoa* color_panel =
      [[ColorPanelCocoa alloc] initWithChooser:&color_chooser_mac];

  EXPECT_TRUE(color_panel);

  NSColorPanel* nscolor_panel = [NSColorPanel sharedColorPanel];

  EXPECT_TRUE([nscolor_panel delegate] == color_panel);
  EXPECT_TRUE([nscolor_panel respondsToSelector:@selector(__target)]);
  EXPECT_TRUE([nscolor_panel __target] == color_panel);

  [color_panel release];

  EXPECT_TRUE([nscolor_panel delegate] == nil);
  EXPECT_TRUE([nscolor_panel __target] == nil);

  // Confirm that the NSColorPanel's target gets cleared on ColorPanelCocoa
  // dealloc if the NSColorPanel's delegate is cleared separately.
  color_panel = [[ColorPanelCocoa alloc] initWithChooser:&color_chooser_mac];

  EXPECT_TRUE(color_panel);

  EXPECT_TRUE([nscolor_panel delegate] == color_panel);
  EXPECT_TRUE([nscolor_panel __target] == color_panel);

  [nscolor_panel setDelegate:nil];

  [color_panel release];

  EXPECT_TRUE([nscolor_panel delegate] == nil);
  EXPECT_TRUE([nscolor_panel __target] == nil);

  [nscolor_panel orderOut:nil];
}

TEST_F(ColorPanelCocoaTest, SetColor) {}

}  // namespace
