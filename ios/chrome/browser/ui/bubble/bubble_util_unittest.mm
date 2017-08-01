// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_util.h"

#include "base/i18n/rtl.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const CGFloat kTestBubbleAlignmentOffset = 10.0f;
}  // namespace

class BubbleUtilTest : public PlatformTest {
 public:
  BubbleUtilTest()
      : leftAlignedTarget_(CGRectMake(20.0f, 200.0f, 100.0f, 100.0f)),
        centerAlignedTarget_(CGRectMake(250.0f, 200.0f, 100.0f, 100.0f)),
        rightAlignedTarget_(CGRectMake(400.0f, 200.0f, 100.0f, 100.0f)),
        bubbleSize_(CGSizeMake(300.0f, 100.0f)),
        containerWidth_(500.0f) {}

 protected:
  // Target element on the left side of the container.
  const CGRect leftAlignedTarget_;
  // Target element on the center of the container.
  const CGRect centerAlignedTarget_;
  // Target element on the right side of the container.
  const CGRect rightAlignedTarget_;
  const CGSize bubbleSize_;
  // Bounding width of the bubble's coordinate system.
  const CGFloat containerWidth_;
};

// Test the |MaxWidth| method when the bubble is leading aligned, the target is
// on the left side of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthLeadingWithTargetOnLeft) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(leftAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat leftAlignedWidth = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(430.0f + kTestBubbleAlignmentOffset, leftAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is leading aligned, the target is
// on the center of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthLeadingWithTargetOnCenter) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat centerAlignedWidth = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(200.0f + kTestBubbleAlignmentOffset, centerAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is leading aligned, the target is
// on the right side of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthLeadingWithTargetOnRight) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(rightAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat rightAlignedWidth = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(50.0f + kTestBubbleAlignmentOffset, rightAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is center aligned, the target is
// on the left side of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthCenterWithTargetOnLeft) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(leftAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat leftAlignedWidth = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(140.0f, leftAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is center aligned, the target is
// on the center of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthCenterWithTargetOnCenter) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat centerAlignedWidth = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(400.0f, centerAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is center aligned, the target is
// on the right side of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthCenterWithTargetOnRight) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(rightAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat rightAlignedWidth = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(100.0f, rightAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is trailing aligned, the target is
// on the left side of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthTrailingWithTargetOnLeft) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(leftAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat leftAlignedWidth = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(70.0f + kTestBubbleAlignmentOffset, leftAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is trailing aligned, the target is
// on the center of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthTrailingWithTargetOnCenter) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat centerAlignedWidth = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(300.0f + kTestBubbleAlignmentOffset, centerAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is trailing aligned, the target is
// on the right side of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthTrailingWithTargetOnRight) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(rightAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat rightAlignedWidth = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(450.0f + kTestBubbleAlignmentOffset, rightAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is leading aligned, the target is
// on the left side of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthLeadingWithTargetOnLeftRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(leftAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat leftAlignedWidthRTL = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(70.0f + kTestBubbleAlignmentOffset, leftAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is leading aligned, the target is
// on the center of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthLeadingWithTargetOnCenterRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat centerAlignedWidthRTL = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(300.0f + kTestBubbleAlignmentOffset, centerAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is leading aligned, the target is
// on the right side of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthLeadingWithTargetOnRightRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(rightAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat rightAlignedWidthRTL = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(450.0f + kTestBubbleAlignmentOffset, rightAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is center aligned, the target is
// on the left side of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthCenterWithTargetOnLeftRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(leftAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat leftAlignedWidthRTL = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(140.0f, leftAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is center aligned, the target is
// on the center of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthCenterWithTargetOnCenterRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat centerAlignedWidthRTL = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(400.0f, centerAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is center aligned, the target is
// on the right side of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthCenterWithTargetOnRightRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(rightAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat rightAlignedWidthRTL = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(100.0f, rightAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is trailing aligned, the target is
// on the left side of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthTrailingWithTargetOnLeftRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(leftAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat leftAlignedWidthRTL = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(430.0f + kTestBubbleAlignmentOffset, leftAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is trailing aligned, the target is
// on the center of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthTrailingWithTargetOnCenterRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat centerAlignedWidthRTL = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(200.0f + kTestBubbleAlignmentOffset, centerAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is trailing aligned, the target is
// on the right side of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthTrailingWithTargetOnRightRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(rightAlignedTarget_, BubbleArrowDirectionDown);
  CGFloat rightAlignedWidthRTL = bubble_util::MaxWidth(
      anchorPoint, BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(50.0f + kTestBubbleAlignmentOffset, rightAlignedWidthRTL);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionUp|, the alignment is |BubbleAlignmentLeading|, and the
// language is LTR
TEST_F(BubbleUtilTest, BubbleFrameUpLeadingLTR) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionUp);
  CGRect bubbleFrame =
      bubble_util::BubbleFrame(anchorPoint, bubbleSize_, BubbleArrowDirectionUp,
                               BubbleAlignmentLeading, containerWidth_);
  CGRect expectedRect = CGRectMake(300.0f - kTestBubbleAlignmentOffset, 300.0f,
                                   bubbleSize_.width, bubbleSize_.height);
  EXPECT_EQ(expectedRect.origin.x, bubbleFrame.origin.x);
  EXPECT_EQ(expectedRect.origin.y, bubbleFrame.origin.y);
  EXPECT_EQ(expectedRect.size.width, bubbleFrame.size.width);
  EXPECT_EQ(expectedRect.size.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionUp|, the alignment is |BubbleAlignmentLeading|, and the
// language is RTL
TEST_F(BubbleUtilTest, BubbleFrameUpLeadingRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionUp);
  CGRect bubbleFrame =
      bubble_util::BubbleFrame(anchorPoint, bubbleSize_, BubbleArrowDirectionUp,
                               BubbleAlignmentLeading, containerWidth_);
  CGRect expectedRect = CGRectMake(-100.0f + kTestBubbleAlignmentOffset, 300.0f,
                                   bubbleSize_.width, bubbleSize_.height);
  EXPECT_EQ(expectedRect.origin.x, bubbleFrame.origin.x);
  EXPECT_EQ(expectedRect.origin.y, bubbleFrame.origin.y);
  EXPECT_EQ(expectedRect.size.width, bubbleFrame.size.width);
  EXPECT_EQ(expectedRect.size.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionUp|, the alignment is |BubbleAlignmentCenter|, and the
// language is LTR
TEST_F(BubbleUtilTest, BubbleFrameUpCenteredLTR) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionUp);
  CGRect bubbleFrame =
      bubble_util::BubbleFrame(anchorPoint, bubbleSize_, BubbleArrowDirectionUp,
                               BubbleAlignmentCenter, containerWidth_);
  CGRect expectedRect =
      CGRectMake(150.0f, 300.0f, bubbleSize_.width, bubbleSize_.height);
  EXPECT_EQ(expectedRect.origin.x, bubbleFrame.origin.x);
  EXPECT_EQ(expectedRect.origin.y, bubbleFrame.origin.y);
  EXPECT_EQ(expectedRect.size.width, bubbleFrame.size.width);
  EXPECT_EQ(expectedRect.size.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionUp|, the alignment is |BubbleAlignmentCenter|, and the
// language is RTL
TEST_F(BubbleUtilTest, BubbleFrameUpCenteredRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionUp);
  CGRect bubbleFrame =
      bubble_util::BubbleFrame(anchorPoint, bubbleSize_, BubbleArrowDirectionUp,
                               BubbleAlignmentCenter, containerWidth_);
  CGRect expectedRect =
      CGRectMake(50.0f, 300.0f, bubbleSize_.width, bubbleSize_.height);
  EXPECT_EQ(expectedRect.origin.x, bubbleFrame.origin.x);
  EXPECT_EQ(expectedRect.origin.y, bubbleFrame.origin.y);
  EXPECT_EQ(expectedRect.size.width, bubbleFrame.size.width);
  EXPECT_EQ(expectedRect.size.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionUp|, the alignment is |BubbleAlignmentTrailing|, and the
// language is LTR
TEST_F(BubbleUtilTest, BubbleFrameUpTrailingLTR) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionUp);
  CGRect bubbleFrame =
      bubble_util::BubbleFrame(anchorPoint, bubbleSize_, BubbleArrowDirectionUp,
                               BubbleAlignmentTrailing, containerWidth_);
  CGRect expectedRect = CGRectMake(kTestBubbleAlignmentOffset, 300.0f,
                                   bubbleSize_.width, bubbleSize_.height);
  EXPECT_EQ(expectedRect.origin.x, bubbleFrame.origin.x);
  EXPECT_EQ(expectedRect.origin.y, bubbleFrame.origin.y);
  EXPECT_EQ(expectedRect.size.width, bubbleFrame.size.width);
  EXPECT_EQ(expectedRect.size.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionUp|, the alignment is |BubbleAlignmentTrailing|, and the
// language is RTL
TEST_F(BubbleUtilTest, BubbleFrameUpTrailingRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionUp);
  CGRect bubbleFrame =
      bubble_util::BubbleFrame(anchorPoint, bubbleSize_, BubbleArrowDirectionUp,
                               BubbleAlignmentTrailing, containerWidth_);
  CGRect expectedRect = CGRectMake(200.0f - kTestBubbleAlignmentOffset, 300.0f,
                                   bubbleSize_.width, bubbleSize_.height);
  EXPECT_EQ(expectedRect.origin.x, bubbleFrame.origin.x);
  EXPECT_EQ(expectedRect.origin.y, bubbleFrame.origin.y);
  EXPECT_EQ(expectedRect.size.width, bubbleFrame.size.width);
  EXPECT_EQ(expectedRect.size.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionDown|, the alignment is |BubbleAlignmentLeading|, and
// the language is LTR
TEST_F(BubbleUtilTest, BubbleFrameDownLeadingLTR) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionDown);
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      anchorPoint, bubbleSize_, BubbleArrowDirectionDown,
      BubbleAlignmentLeading, containerWidth_);
  CGRect expectedRect = CGRectMake(300.0f - kTestBubbleAlignmentOffset, 100.0f,
                                   bubbleSize_.width, bubbleSize_.height);
  EXPECT_EQ(expectedRect.origin.x, bubbleFrame.origin.x);
  EXPECT_EQ(expectedRect.origin.y, bubbleFrame.origin.y);
  EXPECT_EQ(expectedRect.size.width, bubbleFrame.size.width);
  EXPECT_EQ(expectedRect.size.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionDown|, the alignment is |BubbleAlignmentLeading|, and
// the language is RTL
TEST_F(BubbleUtilTest, BubbleFrameDownLeadingRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionDown);
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      anchorPoint, bubbleSize_, BubbleArrowDirectionDown,
      BubbleAlignmentLeading, containerWidth_);
  CGRect expectedRect = CGRectMake(-100.0f + kTestBubbleAlignmentOffset, 100.0f,
                                   bubbleSize_.width, bubbleSize_.height);
  EXPECT_EQ(expectedRect.origin.x, bubbleFrame.origin.x);
  EXPECT_EQ(expectedRect.origin.y, bubbleFrame.origin.y);
  EXPECT_EQ(expectedRect.size.width, bubbleFrame.size.width);
  EXPECT_EQ(expectedRect.size.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionDown|, the alignment is |BubbleAlignmentCenter|, and the
// language is LTR
TEST_F(BubbleUtilTest, BubbleFrameDownCenteredLTR) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionDown);
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      anchorPoint, bubbleSize_, BubbleArrowDirectionDown, BubbleAlignmentCenter,
      containerWidth_);
  CGRect expectedRect =
      CGRectMake(150.0f, 100.0f, bubbleSize_.width, bubbleSize_.height);
  EXPECT_EQ(expectedRect.origin.x, bubbleFrame.origin.x);
  EXPECT_EQ(expectedRect.origin.y, bubbleFrame.origin.y);
  EXPECT_EQ(expectedRect.size.width, bubbleFrame.size.width);
  EXPECT_EQ(expectedRect.size.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionDown|, the alignment is |BubbleAlignmentCenter|, and the
// language is RTL
TEST_F(BubbleUtilTest, BubbleFrameDownCenteredRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionDown);
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      anchorPoint, bubbleSize_, BubbleArrowDirectionDown, BubbleAlignmentCenter,
      containerWidth_);
  CGRect expectedRect =
      CGRectMake(50.0f, 100.0f, bubbleSize_.width, bubbleSize_.height);
  EXPECT_EQ(expectedRect.origin.x, bubbleFrame.origin.x);
  EXPECT_EQ(expectedRect.origin.y, bubbleFrame.origin.y);
  EXPECT_EQ(expectedRect.size.width, bubbleFrame.size.width);
  EXPECT_EQ(expectedRect.size.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionDown|, the alignment is |BubbleAlignmentTrailing|, and
// the language is LTR
TEST_F(BubbleUtilTest, BubbleFrameDownTrailingLTR) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionDown);
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      anchorPoint, bubbleSize_, BubbleArrowDirectionDown,
      BubbleAlignmentTrailing, containerWidth_);
  CGRect expectedRect = CGRectMake(kTestBubbleAlignmentOffset, 100.0f,
                                   bubbleSize_.width, bubbleSize_.height);
  EXPECT_EQ(expectedRect.origin.x, bubbleFrame.origin.x);
  EXPECT_EQ(expectedRect.origin.y, bubbleFrame.origin.y);
  EXPECT_EQ(expectedRect.size.width, bubbleFrame.size.width);
  EXPECT_EQ(expectedRect.size.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionDown|, the alignment is |BubbleAlignmentTrailing|, and
// the language is RTL
TEST_F(BubbleUtilTest, BubbleFrameDownTrailingRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGPoint anchorPoint =
      bubble_util::AnchorPoint(centerAlignedTarget_, BubbleArrowDirectionDown);
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      anchorPoint, bubbleSize_, BubbleArrowDirectionDown,
      BubbleAlignmentTrailing, containerWidth_);
  CGRect expectedRect = CGRectMake(200.0f - kTestBubbleAlignmentOffset, 100.0f,
                                   bubbleSize_.width, bubbleSize_.height);
  EXPECT_EQ(expectedRect.origin.x, bubbleFrame.origin.x);
  EXPECT_EQ(expectedRect.origin.y, bubbleFrame.origin.y);
  EXPECT_EQ(expectedRect.size.width, bubbleFrame.size.width);
  EXPECT_EQ(expectedRect.size.height, bubbleFrame.size.height);
}

// Test the |DivideRectAtPoint| method when the bubble's arrow is facing up
// (which means the bubble itself is below the anchor point).
TEST_F(BubbleUtilTest, DivideRectDirectionUp) {
  CGRect sourceRect = CGRectMake(0, 0, 100, 100);
  CGPoint anchorPoint = CGPointMake(30, 40);

  CGRect dividedRect = bubble_util::DivideRectAtPoint(sourceRect, anchorPoint,
                                                      BubbleArrowDirectionUp);

  CGRect expectedRect = CGRectMake(0.0f, 40.0f, 100.0f, 60.0f);
  EXPECT_EQ(expectedRect.origin.x, dividedRect.origin.x);
  EXPECT_EQ(expectedRect.origin.y, dividedRect.origin.y);
  EXPECT_EQ(expectedRect.size.width, dividedRect.size.width);
  EXPECT_EQ(expectedRect.size.height, dividedRect.size.height);
}

// Test the |DivideRectAtPoint| method when the bubble's arrow is facing down
// (which means the bubble itself is above the anchor point).
TEST_F(BubbleUtilTest, DivideRectDirectionDown) {
  CGRect sourceRect = CGRectMake(0, 0, 100, 100);
  CGPoint anchorPoint = CGPointMake(30, 40);

  CGRect dividedRect = bubble_util::DivideRectAtPoint(sourceRect, anchorPoint,
                                                      BubbleArrowDirectionDown);

  CGRect expectedRect = CGRectMake(0.0f, 0.0f, 100.0f, 40.0f);
  EXPECT_EQ(expectedRect.origin.x, dividedRect.origin.x);
  EXPECT_EQ(expectedRect.origin.y, dividedRect.origin.y);
  EXPECT_EQ(expectedRect.size.width, dividedRect.size.width);
  EXPECT_EQ(expectedRect.size.height, dividedRect.size.height);
}

// Test the |DivideRectAtPoint| method when the bubble's arrow is facing up
// (which means the bubble itself is below the anchor point) and the rect to be
// divided is not at the origin.
TEST_F(BubbleUtilTest, DivideRectDirectionUpNotAtOrigin) {
  CGRect sourceRect = CGRectMake(20, 30, 80, 50);
  CGPoint anchorPoint = CGPointMake(30, 40);

  CGRect dividedRect = bubble_util::DivideRectAtPoint(sourceRect, anchorPoint,
                                                      BubbleArrowDirectionUp);

  CGRect expectedRect = CGRectMake(20.0f, 40.0f, 80.0f, 40.0f);
  EXPECT_EQ(expectedRect.origin.x, dividedRect.origin.x);
  EXPECT_EQ(expectedRect.origin.y, dividedRect.origin.y);
  EXPECT_EQ(expectedRect.size.width, dividedRect.size.width);
  EXPECT_EQ(expectedRect.size.height, dividedRect.size.height);
}

// Test the |DivideRectAtPoint| method when the bubble's arrow is facing down
// (which means the bubble itself is above the anchor point) and the rect to be
// divided is not at the origin.
TEST_F(BubbleUtilTest, DivideRectDirectionDownNotAtOrigin) {
  CGRect sourceRect = CGRectMake(20, 30, 80, 50);
  CGPoint anchorPoint = CGPointMake(30, 40);

  CGRect dividedRect = bubble_util::DivideRectAtPoint(sourceRect, anchorPoint,
                                                      BubbleArrowDirectionDown);

  CGRect expectedRect = CGRectMake(20.0f, 30.0f, 80.0f, 10.0f);
  EXPECT_EQ(expectedRect.origin.x, dividedRect.origin.x);
  EXPECT_EQ(expectedRect.origin.y, dividedRect.origin.y);
  EXPECT_EQ(expectedRect.size.width, dividedRect.size.width);
  EXPECT_EQ(expectedRect.size.height, dividedRect.size.height);
}
