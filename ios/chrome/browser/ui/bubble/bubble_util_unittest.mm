// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test that the BubbleAnchorPoint util method returns the bottom-midpoint of
// the target UI element when the bubble's arrow direction is up.
TEST(BubbleUtilTest, AnchorPointUpArrowTest) {}

// Test that the BubbleAnchorPoint util method returns the top-midpoint of the
// target UI element when the bubble's arrow direction is down.
TEST(BubbleUtilTest, AnchorPointDownArrowTest) {}

// Test the y-coordinate returned by the BubbleOrigin util method when the
// bubble's arrow direction is up. The origin's y-coordinate should be the
// bottom of the target element.
TEST(BubbleUtilTest, OriginUpArrowTest) {}

// Test the y-coordinate returned by the BubbleOrigin util method when the
// bubble's arrow direction is down. The origin's y-coordinate should be the
// origin of the target element minus the bubble's height.
TEST(BubbleUtilTest, OriginDownArrowTest) {}

// Test that the x-coordinate returned by the BubbleOrigin util method aligns
// the bubble's arrow with the target element, given leading alignment and
// bubble size.
TEST(BubbleUtilTest, OriginLeadingAlignmentTest) {}

// Test the x-coordinate returned by the BubbleOrigin util method when the
// bubble is center aligned.
TEST(BubbleUtilTest, OriginCenterAlignmentTest) {}

// Test the x-coordinate returned by the BubbleOrigin util method when the
// bubble is trailing aligned.
TEST(BubbleUtilTest, OriginTrailingAlignmentTest) {}
