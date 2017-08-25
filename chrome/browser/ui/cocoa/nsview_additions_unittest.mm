// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "chrome/browser/ui/cocoa/nsview_additions.h"

#import <utility>
#import <vector>

#import "chrome/browser/ui/cocoa/test/cocoa_test_helper.h"
#import "chrome/browser/ui/cocoa/test/scoped_force_rtl_mac.h"

namespace {

class NSViewAdditionsTest : public ui::CocoaTest {};

TEST_F(NSViewAdditionsTest, LocalizedAutoresizingMask) {
  std::vector<std::pair<NSAutoresizingMaskOptions, NSAutoresizingMaskOptions>>
      testCases{
          // A mask without x margin options shouldn't be changed.
          {NSViewWidthSizable | NSViewHeightSizable,
           NSViewWidthSizable | NSViewHeightSizable},

          // …nor should a mask with both x margin options.
          {NSViewMinXMargin | NSViewMaxXMargin,
           NSViewMinXMargin | NSViewMaxXMargin},

          // MinXMargin becomes MaxXMargin,
          {NSViewMinXMargin, NSViewMaxXMargin},

          // MaxXMargin becomes MinXMargin.
          {NSViewMaxXMargin, NSViewMinXMargin},

          // Y margins should be left alone.
          {NSViewMinYMargin | NSViewMinXMargin,
           NSViewMinYMargin | NSViewMaxXMargin},
      };

  for (const auto& pair : testCases) {
    EXPECT_EQ(pair.first, [NSView cr_localizedAutoresizingMask:pair.first]);
  }

  cocoa_l10n_util::ScopedForceRTLMac rtl;

  for (const auto& pair : testCases) {
    EXPECT_EQ(pair.second, [NSView cr_localizedAutoresizingMask:pair.first]);
  }
}

TEST_F(NSViewAdditionsTest, LocalizedRect) {
  NSView* contentView = test_window().contentView;
  NSRect rect = NSMakeRect(0, 0, 10, 10);
  EXPECT_TRUE(NSEqualRects(rect, [contentView cr_localizedRect:rect]));

  cocoa_l10n_util::ScopedForceRTLMac rtl;

  EXPECT_TRUE(
      NSEqualRects(NSMakeRect(NSMaxX(contentView.bounds) - 10, 0, 10, 10),
                   [test_window().contentView cr_localizedRect:rect]));
}

}  // namespace
