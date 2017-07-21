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

// Fixture to test TextBadgeView.
class TextBadgeViewTest : public PlatformTest {};

// Tests the badge's intrinsic content size given short display text.
TEST_F(TextBadgeViewTest, BadgeSizeShortLabel) {}

// Tests the badge's intrinsic content size given long display text.
TEST_F(TextBadgeViewTest, BadgeSizeLongLabel) {}

// Tests that text and layout flip for RTL languages.
TEST_F(TextBadgeViewTest, RTL) {}

// Tests that the accessibility label matches the display text.
TEST_F(TextBadgeViewTest, Accessibility) {}
