// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/color_chooser_mac.h"

#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#include "skia/ext/skia_utils_mac.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

@interface NSColorPanel (Private)
// Private method returning the NSColorPanel's target.
- (id)__target;
@end

@interface ColorPanelCocoaForTesting : ColorPanelCocoa
@property(nonatomic) BOOL colorChangeWasNonUserChange;
@end

@implementation ColorPanelCocoaForTesting
@synthesize colorChangeWasNonUserChange = colorChangeWasNonUserChange_;

- (void)didChooseColor:(NSColorPanel*)panel {
  colorChangeWasNonUserChange_ = nonUserChange_;
  [super didChooseColor:panel];
}

@end

namespace {

class ColorPanelCocoaTest : public CocoaTest {};

TEST_F(ColorPanelCocoaTest, Dealloc) {
  // Create a ColorPanelCocoa.
  ColorChooserMac color_chooser_mac(nullptr, SK_ColorBLACK);

  base::scoped_nsobject<ColorPanelCocoa> color_panel_cocoa(
      [[ColorPanelCocoa alloc] initWithChooser:&color_chooser_mac]);

  EXPECT_TRUE(color_panel_cocoa.get());

  // Get the NSColorPanel and confirm it's configuration by ColorPanelCocoa.
  NSColorPanel* nscolor_panel = [NSColorPanel sharedColorPanel];

  EXPECT_EQ([nscolor_panel delegate], color_panel_cocoa.get());
  EXPECT_TRUE([nscolor_panel respondsToSelector:@selector(__target)]);
  EXPECT_EQ([nscolor_panel __target], color_panel_cocoa.get());

  // Release the ColorPanelCocoa and confirm it's no longer the NSColorPanel's
  // target or delegate.
  color_panel_cocoa.reset();

  EXPECT_EQ([nscolor_panel delegate], nil);
  EXPECT_EQ([nscolor_panel __target], nil);

  // Create a new ColorPanelCocoa and confirm the NSColorPanel's configuration.
  color_panel_cocoa.reset(
      [[ColorPanelCocoa alloc] initWithChooser:&color_chooser_mac]);

  EXPECT_TRUE(color_panel_cocoa.get());

  EXPECT_EQ([nscolor_panel delegate], color_panel_cocoa.get());
  EXPECT_EQ([nscolor_panel __target], color_panel_cocoa.get());

  // Clear the delegate and release the ColorPanelCocoa.
  [nscolor_panel setDelegate:nil];

  color_panel_cocoa.reset();

  // Confirm the ColorPanelCocoa is no longer the NSColorPanel's target or
  // delegate. Previously the ColorPanelCocoa would not clear the target if
  // the delegate had already been cleared.
  EXPECT_EQ([nscolor_panel delegate], nil);
  EXPECT_EQ([nscolor_panel __target], nil);
}

TEST_F(ColorPanelCocoaTest, SetColor) {
  // Set the NSColor panel up with an intial color.
  NSColor* blue_color = [NSColor blueColor];
  NSColorPanel* nscolor_panel = [NSColorPanel sharedColorPanel];
  [nscolor_panel setColor:blue_color];
  EXPECT_TRUE([[nscolor_panel color] isEqual:blue_color]);

  // Create a ColorChooserMac and confirm the NSColorPanel gets its initial
  // color.
  SkColor initial_color = SK_ColorBLACK;
  ColorChooserMac color_chooser_mac(nullptr, initial_color);

  EXPECT_NSEQ([nscolor_panel color],
              skia::SkColorToDeviceNSColor(initial_color));

  base::scoped_nsobject<ColorPanelCocoa> color_panel_cocoa(
      [[ColorPanelCocoa alloc] initWithChooser:&color_chooser_mac]);

  EXPECT_TRUE(color_panel_cocoa.get());

  // Confirm that -[ColorPanelCocoa setColor:] sets the NSColorPanel's color.
  NSColor* test_color = [NSColor redColor];
  [color_panel_cocoa setColor:test_color];

  EXPECT_NSEQ([nscolor_panel color], test_color);
}

TEST_F(ColorPanelCocoaTest, UserVsProgrammaticColorChanges) {
  // Create a ColorPanelCocoaForTesting.
  ColorChooserMac color_chooser_mac(nullptr, SK_ColorBLACK);

  base::scoped_nsobject<ColorPanelCocoaForTesting> color_panel_cocoa(
      [[ColorPanelCocoaForTesting alloc] initWithChooser:&color_chooser_mac]);

  EXPECT_TRUE(color_panel_cocoa.get());

  // Setting the NSColorPanel's color should register as a user-initiated
  // change.
  NSColorPanel* nscolor_panel = [NSColorPanel sharedColorPanel];
  [nscolor_panel setColor:[NSColor redColor]];

  EXPECT_FALSE([color_panel_cocoa colorChangeWasNonUserChange]);

  // Setting the ColorPanelCocoa's color should register as a programmatic
  // change.
  [color_panel_cocoa setColor:[NSColor blueColor]];

  EXPECT_TRUE([color_panel_cocoa colorChangeWasNonUserChange]);
}

}  // namespace
