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

// Test that the origin's y-coordinate is |anchorPoint.y| when |arrowDirection|
// is |BubbleArrowDirectionUp|.
TEST(BubbleUtilTest, OriginUpArrowTest) {}

// Test that the origin's y-coordinate is |anchorPoint.y - size.height| when
// |arrowDirection| is |BubbleArrowDirectionDown|.
TEST(BubbleUtilTest, OriginDownArrowTest) {}

// Test that the origin's x-coordinate corresponds to a center-aligned bubble
// when center alignment would not cause the bubble to exceed |maxFrame|.
TEST(BubbleUtilTest, OriginCenterAlignmentTest) {}

// Test that the origin's x-coordinate corresponds to a left-aligned bubble when
// center alignment would cause the bubble's left edge to be left of
// |maxFrame|'s left edge.
TEST(BubbleUtilTest, OriginLeftAlignmentTest) {}

// Test that the origin's x-coordinate corresponds to a right-aligned bubble
// when center alignment would cause the bubble's right edge to be right of
// |maxFrame|'s right edge.
TEST(BubbleUtilTest, OriginRightAlignmentTest) {}
