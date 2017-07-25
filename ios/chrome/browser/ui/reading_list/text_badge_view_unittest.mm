// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/text_badge_view.h"

#import <Foundation/Foundation.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Tests setting the |text| property.
TEST(TextBadgeViewTest, SetText) {
  NSString* text1 = @"text 1";
  NSString* text2 = @"text 2";
  TextBadgeView* badge = [[TextBadgeView alloc] initWithText:text1];
  [badge setText:text2];
  EXPECT_EQ(text2, badge.text);
}

// Tests that the accessibility label matches the display text.
TEST(TextBadgeViewTest, Accessibility) {
  NSString* text = @"display";
  TextBadgeView* badge = [[TextBadgeView alloc] initWithText:text];
  NSString* a11yLabel = badge.accessibilityLabel;
  EXPECT_EQ(text, a11yLabel);
}
