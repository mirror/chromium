// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_util.h"

#include "base/i18n/rtl.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

CGFloat kTestBubbleAlignmentOffset = 10.0f;

// Override the current text direction to test RTL conditions.
void SetRTL(bool rtl) {
  base::i18n::SetICUDefaultLocale(rtl ? "he" : "en");
  EXPECT_EQ(rtl, base::i18n::IsRTL());
}
};

// Test that the y-coordinate returned by the |OriginY| method when the bubble's
// arrow direction is up is the bottom of the target element.
TEST(BubbleUtilTest, OriginUpArrowTest) {
  CGRect target = CGRectMake(20.0f, 20.0f, 100.0f, 100.0f);
  CGFloat originY =
      bubble_util::OriginY(target, BubbleArrowDirectionUp, 100.0f);
  EXPECT_EQ(originY, 120.0f);
}

// Test that the y-coordinate returned by the |OriginY| method when the
// bubble's arrow direction is down is the y-coordinate of the target element's
// origin minus the bubble's height.
TEST(BubbleUtilTest, OriginDownArrowTest) {
  CGRect target = CGRectMake(20.0f, 300.0f, 100.0f, 100.0f);
  CGFloat originY =
      bubble_util::OriginY(target, BubbleArrowDirectionDown, 100.0f);
  EXPECT_EQ(originY, 200.0f);
}

// Test the |LeadingDistance| method when the bubble is leading aligned.
// Positioning the bubble |LeadingDistance| from the leading edge of its
// superview should align the bubble's arrow with the target element.
TEST(BubbleUtilTest, LeadingDistanceLeadingAlignmentTest) {
  CGRect target = CGRectMake(20.0f, 20.0f, 100.0f, 100.0f);
  CGFloat leading =
      bubble_util::LeadingDistance(target, BubbleAlignmentLeading, 300.0f);
  EXPECT_EQ(leading, 70.0f - kTestBubbleAlignmentOffset);
}

// Test the |LeadingDistance| method when the bubble is center aligned.
TEST(BubbleUtilTest, LeadingDistanceCenterAlignmentTest) {
  CGRect target = CGRectMake(200.0f, 20.0f, 100.0f, 100.0f);
  CGFloat leading =
      bubble_util::LeadingDistance(target, BubbleAlignmentCenter, 300.0f);
  EXPECT_EQ(leading, 100.0f);
}

// Test the |LeadingDistance| method when the bubble is trailing aligned.
TEST(BubbleUtilTest, LeadingDistanceTrailingAlignmentTest) {
  CGRect target = CGRectMake(300.0f, 20.0f, 100.0f, 100.0f);
  CGFloat leading =
      bubble_util::LeadingDistance(target, BubbleAlignmentTrailing, 300.0f);
  EXPECT_EQ(leading, 50.0f + kTestBubbleAlignmentOffset);
}

// Test the |MaxWidth| method when the bubble is leading aligned and the
// language is LTR.
TEST(BubbleUtilTest, MaxWidthLeadingAlignmentLTRTest) {
  BubbleAlignment alignment = BubbleAlignmentLeading;
  SetRTL(NO);
  CGRect leftAlignedTarget = CGRectMake(20.0f, 20.0f, 100.0f, 100.0f);
  CGFloat leftAlignedWidthLTR =
      bubble_util::MaxWidth(leftAlignedTarget, alignment, 500.0f);
  EXPECT_EQ(leftAlignedWidthLTR, 430.0f + kTestBubbleAlignmentOffset);

  CGRect centerAlignedTarget = CGRectMake(250.0f, 20.0f, 100.0f, 100.0f);
  CGFloat centerAlignedWidthLTR =
      bubble_util::MaxWidth(centerAlignedTarget, alignment, 500.0f);
  EXPECT_EQ(centerAlignedWidthLTR, 200.0f + kTestBubbleAlignmentOffset);

  CGRect rightAlignedTarget = CGRectMake(400.0f, 20.0f, 100.0f, 100.0f);
  CGFloat rightAlignedWidthLTR =
      bubble_util::MaxWidth(rightAlignedTarget, alignment, 500.0f);
  EXPECT_EQ(rightAlignedWidthLTR, 50.0f + kTestBubbleAlignmentOffset);
}

// Test the |MaxWidth| method when the bubble is leading aligned and the
// language is RTL.
TEST(BubbleUtilTest, MaxWidthLeadingAlignmentRTLTest) {
  BubbleAlignment alignment = BubbleAlignmentLeading;
  SetRTL(YES);
  CGRect leftAlignedTarget = CGRectMake(20.0f, 20.0f, 100.0f, 100.0f);
  CGFloat leftAlignedWidthRTL =
      bubble_util::MaxWidth(leftAlignedTarget, alignment, 500.0f);
  EXPECT_EQ(leftAlignedWidthRTL, 70.0f + kTestBubbleAlignmentOffset);

  CGRect centerAlignedTarget = CGRectMake(250.0f, 20.0f, 100.0f, 100.0f);
  CGFloat centerAlignedWidthRTL =
      bubble_util::MaxWidth(centerAlignedTarget, alignment, 500.0f);
  EXPECT_EQ(centerAlignedWidthRTL, 300.0f + kTestBubbleAlignmentOffset);

  CGRect rightAlignedTarget = CGRectMake(400.0f, 20.0f, 100.0f, 100.0f);
  CGFloat rightAlignedWidthRTL =
      bubble_util::MaxWidth(rightAlignedTarget, alignment, 500.0f);
  EXPECT_EQ(rightAlignedWidthRTL, 450.0f + kTestBubbleAlignmentOffset);
}

// Test the |MaxWidth| method when the bubble is center aligned.
TEST(BubbleUtilTest, MaxWidthCenterAlignmentTest) {
  BubbleAlignment alignment = BubbleAlignmentCenter;
  // Since a center-aligned bubble is symmetric across the center, the output of
  // |MaxWidth| is independent of language direction.
  for (size_t i = 1; i < 2; ++i) {
    // Change language direction to ensure that both directions yield the same
    // result.
    SetRTL(!base::i18n::IsRTL());
    CGRect leftAlignedTarget = CGRectMake(20.0f, 20.0f, 100.0f, 100.0f);
    CGFloat leftAlignedWidth =
        bubble_util::MaxWidth(leftAlignedTarget, alignment, 500.0f);
    EXPECT_EQ(leftAlignedWidth, 140.0f);

    CGRect centerAlignedTarget = CGRectMake(250.0f, 20.0f, 100.0f, 100.0f);
    CGFloat centerAlignedWidth =
        bubble_util::MaxWidth(centerAlignedTarget, alignment, 500.0f);
    EXPECT_EQ(centerAlignedWidth, 400.0f);

    CGRect rightAlignedTarget = CGRectMake(400.0f, 20.0f, 100.0f, 100.0f);
    CGFloat rightAlignedWidth =
        bubble_util::MaxWidth(rightAlignedTarget, alignment, 500.0f);
    EXPECT_EQ(rightAlignedWidth, 100.0f);
  }
}

// Test the |MaxWidth| method when the bubble is trailing aligned and the
// language is LTR.
TEST(BubbleUtilTest, MaxWidthTrailingAlignmentLTRTest) {
  BubbleAlignment alignment = BubbleAlignmentTrailing;
  SetRTL(NO);
  CGRect leftAlignedTarget = CGRectMake(20.0f, 20.0f, 100.0f, 100.0f);
  CGFloat leftAlignedWidth =
      bubble_util::MaxWidth(leftAlignedTarget, alignment, 500.0f);
  EXPECT_EQ(leftAlignedWidth, 70.0f + kTestBubbleAlignmentOffset);

  CGRect centerAlignedTarget = CGRectMake(250.0f, 20.0f, 100.0f, 100.0f);
  CGFloat centerAlignedWidth =
      bubble_util::MaxWidth(centerAlignedTarget, alignment, 500.0f);
  EXPECT_EQ(centerAlignedWidth, 300.0f + kTestBubbleAlignmentOffset);

  CGRect rightAlignedTarget = CGRectMake(400.0f, 20.0f, 100.0f, 100.0f);
  CGFloat rightAlignedWidth =
      bubble_util::MaxWidth(rightAlignedTarget, alignment, 500.0f);
  EXPECT_EQ(rightAlignedWidth, 450.0f + kTestBubbleAlignmentOffset);
}

// Test the |MaxWidth| method when the bubble is trailing aligned and the
// language is RTL.
TEST(BubbleUtilTest, MaxWidthTrailingAlignmentRTLTest) {
  BubbleAlignment alignment = BubbleAlignmentTrailing;
  SetRTL(YES);
  CGRect leftAlignedTarget = CGRectMake(20.0f, 20.0f, 100.0f, 100.0f);
  CGFloat leftAlignedWidthRTL =
      bubble_util::MaxWidth(leftAlignedTarget, alignment, 500.0f);
  EXPECT_EQ(leftAlignedWidthRTL, 430.0f + kTestBubbleAlignmentOffset);

  CGRect centerAlignedTarget = CGRectMake(250.0f, 20.0f, 100.0f, 100.0f);
  CGFloat centerAlignedWidthRTL =
      bubble_util::MaxWidth(centerAlignedTarget, alignment, 500.0f);
  EXPECT_EQ(centerAlignedWidthRTL, 200.0f + kTestBubbleAlignmentOffset);

  CGRect rightAlignedTarget = CGRectMake(400.0f, 20.0f, 100.0f, 100.0f);
  CGFloat rightAlignedWidthRTL =
      bubble_util::MaxWidth(rightAlignedTarget, alignment, 500.0f);
  EXPECT_EQ(rightAlignedWidthRTL, 50.0f + kTestBubbleAlignmentOffset);
}
