// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test that the anchor point is the bottom-midpoint of |frame| when
// |arrowDirection| is |BubbleArrowDirectionUp|.
TEST(BubbleUtilTest, AnchorPointUpArrowTest) {}

// Test that the anchor point is the top-midpoint of |frame| when
// |arrowDirection| is |BubbleArrowDirectionDown|.
TEST(BubbleUtilTest, AnchorPointDownArrowTest) {}

// Test the origin's y-coordinate when |arrowDirection| is
// |BubbleArrowDirectionUp|.
TEST(BubbleUtilTest, OriginUpArrowTest) {}

// Test the origin's y-coordinate when |arrowDirection| is
// |BubbleArrowDirectionDown|.
TEST(BubbleUtilTest, OriginDownArrowTest) {}

// Test the origin's x-coordinate when the bubble is center aligned.
TEST(BubbleUtilTest, OriginCenterAlignmentTest) {}

// Test the origin's x-coordinate when the bubble is left aligned.
TEST(BubbleUtilTest, OriginLeftAlignmentTest) {}

// Test the origin's x-coordinate when the bubble is right aligned.
TEST(BubbleUtilTest, OriginRightAlignmentTest) {}
