// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test that the y-coordinate returned by the |OriginY| method when the
// bubble's arrow direction is up is the
// bottom of the target element.
TEST(BubbleUtilTest, OriginUpArrowTest) {}

// Test that the y-coordinate returned by the |OriginY| method when the
// bubble's arrow direction is down is the y-coordinate of the target element's
// origin minus the bubble's height.
TEST(BubbleUtilTest, OriginDownArrowTest) {}

// Test the |LeadingDistance| method when the bubble is leading aligned.
// Positioning the bubble |LeadingDistance| from the leading edge of its
// superview should align the bubble's arrow with the target element.
TEST(BubbleUtilTest, LeadingDistanceLeadingAlignmentTest) {}

// Test the |LeadingDistance| method when the bubble is center aligned.
TEST(BubbleUtilTest, LeadingDistanceCenterAlignmentTest) {}

// Test the |LeadingDistance| method when the bubble is trailing aligned.
TEST(BubbleUtilTest, LeadingDistanceTrailingAlignmentTest) {}
