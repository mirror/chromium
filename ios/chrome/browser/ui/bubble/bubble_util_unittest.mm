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
  CGFloat leftAlignedWidth = bubble_util::MaxWidth(
      leftAlignedTarget_, BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(430.0f + kTestBubbleAlignmentOffset, leftAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is leading aligned, the target is
// on the center of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthLeadingWithTargetOnCenter) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGFloat centerAlignedWidth = bubble_util::MaxWidth(
      centerAlignedTarget_, BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(200.0f + kTestBubbleAlignmentOffset, centerAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is leading aligned, the target is
// on the right side of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthLeadingWithTargetOnRight) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGFloat rightAlignedWidth = bubble_util::MaxWidth(
      rightAlignedTarget_, BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(50.0f + kTestBubbleAlignmentOffset, rightAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is center aligned, the target is
// on the left side of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthCenterWithTargetOnLeft) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGFloat leftAlignedWidth = bubble_util::MaxWidth(
      leftAlignedTarget_, BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(140.0f, leftAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is center aligned, the target is
// on the center of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthCenterWithTargetOnCenter) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGFloat centerAlignedWidth = bubble_util::MaxWidth(
      centerAlignedTarget_, BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(400.0f, centerAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is center aligned, the target is
// on the right side of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthCenterWithTargetOnRight) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGFloat rightAlignedWidth = bubble_util::MaxWidth(
      rightAlignedTarget_, BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(100.0f, rightAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is trailing aligned, the target is
// on the left side of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthTrailingWithTargetOnLeft) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGFloat leftAlignedWidth = bubble_util::MaxWidth(
      leftAlignedTarget_, BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(70.0f + kTestBubbleAlignmentOffset, leftAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is trailing aligned, the target is
// on the center of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthTrailingWithTargetOnCenter) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGFloat centerAlignedWidth = bubble_util::MaxWidth(
      centerAlignedTarget_, BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(300.0f + kTestBubbleAlignmentOffset, centerAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is trailing aligned, the target is
// on the right side of the container, and the language is LTR.
TEST_F(BubbleUtilTest, MaxWidthTrailingWithTargetOnRight) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGFloat rightAlignedWidth = bubble_util::MaxWidth(
      rightAlignedTarget_, BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(450.0f + kTestBubbleAlignmentOffset, rightAlignedWidth);
}

// Test the |MaxWidth| method when the bubble is leading aligned, the target is
// on the left side of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthLeadingWithTargetOnLeftRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGFloat leftAlignedWidthRTL = bubble_util::MaxWidth(
      leftAlignedTarget_, BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(70.0f + kTestBubbleAlignmentOffset, leftAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is leading aligned, the target is
// on the center of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthLeadingWithTargetOnCenterRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGFloat centerAlignedWidthRTL = bubble_util::MaxWidth(
      centerAlignedTarget_, BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(300.0f + kTestBubbleAlignmentOffset, centerAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is leading aligned, the target is
// on the right side of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthLeadingWithTargetOnRightRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGFloat rightAlignedWidthRTL = bubble_util::MaxWidth(
      rightAlignedTarget_, BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(450.0f + kTestBubbleAlignmentOffset, rightAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is center aligned, the target is
// on the left side of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthCenterWithTargetOnLeftRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGFloat leftAlignedWidthRTL = bubble_util::MaxWidth(
      leftAlignedTarget_, BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(140.0f, leftAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is center aligned, the target is
// on the center of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthCenterWithTargetOnCenterRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGFloat centerAlignedWidthRTL = bubble_util::MaxWidth(
      centerAlignedTarget_, BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(400.0f, centerAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is center aligned, the target is
// on the right side of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthCenterWithTargetOnRightRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGFloat rightAlignedWidthRTL = bubble_util::MaxWidth(
      rightAlignedTarget_, BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(100.0f, rightAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is trailing aligned, the target is
// on the left side of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthTrailingWithTargetOnLeftRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGFloat leftAlignedWidthRTL = bubble_util::MaxWidth(
      leftAlignedTarget_, BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(430.0f + kTestBubbleAlignmentOffset, leftAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is trailing aligned, the target is
// on the center of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthTrailingWithTargetOnCenterRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGFloat centerAlignedWidthRTL = bubble_util::MaxWidth(
      centerAlignedTarget_, BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(200.0f + kTestBubbleAlignmentOffset, centerAlignedWidthRTL);
}

// Test the |MaxWidth| method when the bubble is trailing aligned, the target is
// on the right side of the container, and the language is RTL.
TEST_F(BubbleUtilTest, MaxWidthTrailingWithTargetOnRightRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGFloat rightAlignedWidthRTL = bubble_util::MaxWidth(
      rightAlignedTarget_, BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(50.0f + kTestBubbleAlignmentOffset, rightAlignedWidthRTL);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionUp|, the alignment is |BubbleAlignmentLeading|, and the
// language is LTR
TEST_F(BubbleUtilTest, BubbleFrameUpLeadingLTR) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      centerAlignedTarget_, bubbleSize_, BubbleArrowDirectionUp,
      BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(300.0f - kTestBubbleAlignmentOffset, bubbleFrame.origin.x);
  EXPECT_EQ(300.0f, bubbleFrame.origin.y);
  EXPECT_EQ(bubbleSize_.width, bubbleFrame.size.width);
  EXPECT_EQ(bubbleSize_.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionUp|, the alignment is |BubbleAlignmentLeading|, and the
// language is RTL
TEST_F(BubbleUtilTest, BubbleFrameUpLeadingRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      centerAlignedTarget_, bubbleSize_, BubbleArrowDirectionUp,
      BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(-100.0f + kTestBubbleAlignmentOffset, bubbleFrame.origin.x);
  EXPECT_EQ(300.0f, bubbleFrame.origin.y);
  EXPECT_EQ(bubbleSize_.width, bubbleFrame.size.width);
  EXPECT_EQ(bubbleSize_.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionUp|, the alignment is |BubbleAlignmentCenter|, and the
// language is LTR
TEST_F(BubbleUtilTest, BubbleFrameUpCenteredLTR) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      centerAlignedTarget_, bubbleSize_, BubbleArrowDirectionUp,
      BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(150.0f, bubbleFrame.origin.x);
  EXPECT_EQ(300.0f, bubbleFrame.origin.y);
  EXPECT_EQ(bubbleSize_.width, bubbleFrame.size.width);
  EXPECT_EQ(bubbleSize_.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionUp|, the alignment is |BubbleAlignmentCenter|, and the
// language is RTL
TEST_F(BubbleUtilTest, BubbleFrameUpCenteredRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      centerAlignedTarget_, bubbleSize_, BubbleArrowDirectionUp,
      BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(50.0f, bubbleFrame.origin.x);
  EXPECT_EQ(300.0f, bubbleFrame.origin.y);
  EXPECT_EQ(bubbleSize_.width, bubbleFrame.size.width);
  EXPECT_EQ(bubbleSize_.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionUp|, the alignment is |BubbleAlignmentTrailing|, and the
// language is LTR
TEST_F(BubbleUtilTest, BubbleFrameUpTrailingLTR) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      centerAlignedTarget_, bubbleSize_, BubbleArrowDirectionUp,
      BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(kTestBubbleAlignmentOffset, bubbleFrame.origin.x);
  EXPECT_EQ(300.0f, bubbleFrame.origin.y);
  EXPECT_EQ(bubbleSize_.width, bubbleFrame.size.width);
  EXPECT_EQ(bubbleSize_.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionUp|, the alignment is |BubbleAlignmentTrailing|, and the
// language is RTL
TEST_F(BubbleUtilTest, BubbleFrameUpTrailingRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      centerAlignedTarget_, bubbleSize_, BubbleArrowDirectionUp,
      BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(200.0f - kTestBubbleAlignmentOffset, bubbleFrame.origin.x);
  EXPECT_EQ(300.0f, bubbleFrame.origin.y);
  EXPECT_EQ(bubbleSize_.width, bubbleFrame.size.width);
  EXPECT_EQ(bubbleSize_.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionDown|, the alignment is |BubbleAlignmentLeading|, and
// the language is LTR
TEST_F(BubbleUtilTest, BubbleFrameDownLeadingLTR) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      centerAlignedTarget_, bubbleSize_, BubbleArrowDirectionDown,
      BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(300.0f - kTestBubbleAlignmentOffset, bubbleFrame.origin.x);
  EXPECT_EQ(100.0f, bubbleFrame.origin.y);
  EXPECT_EQ(bubbleSize_.width, bubbleFrame.size.width);
  EXPECT_EQ(bubbleSize_.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionDown|, the alignment is |BubbleAlignmentLeading|, and
// the language is RTL
TEST_F(BubbleUtilTest, BubbleFrameDownLeadingRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      centerAlignedTarget_, bubbleSize_, BubbleArrowDirectionDown,
      BubbleAlignmentLeading, containerWidth_);
  EXPECT_EQ(-100.0f + kTestBubbleAlignmentOffset, bubbleFrame.origin.x);
  EXPECT_EQ(100.0f, bubbleFrame.origin.y);
  EXPECT_EQ(bubbleSize_.width, bubbleFrame.size.width);
  EXPECT_EQ(bubbleSize_.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionDown|, the alignment is |BubbleAlignmentCenter|, and the
// language is LTR
TEST_F(BubbleUtilTest, BubbleFrameDownCenteredLTR) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      centerAlignedTarget_, bubbleSize_, BubbleArrowDirectionDown,
      BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(150.0f, bubbleFrame.origin.x);
  EXPECT_EQ(100.0f, bubbleFrame.origin.y);
  EXPECT_EQ(bubbleSize_.width, bubbleFrame.size.width);
  EXPECT_EQ(bubbleSize_.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionDown|, the alignment is |BubbleAlignmentCenter|, and the
// language is RTL
TEST_F(BubbleUtilTest, BubbleFrameDownCenteredRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      centerAlignedTarget_, bubbleSize_, BubbleArrowDirectionDown,
      BubbleAlignmentCenter, containerWidth_);
  EXPECT_EQ(50.0f, bubbleFrame.origin.x);
  EXPECT_EQ(100.0f, bubbleFrame.origin.y);
  EXPECT_EQ(bubbleSize_.width, bubbleFrame.size.width);
  EXPECT_EQ(bubbleSize_.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionDown|, the alignment is |BubbleAlignmentTrailing|, and
// the language is LTR
TEST_F(BubbleUtilTest, BubbleFrameDownTrailingLTR) {
  // Ensure that the language is LTR by setting the locale to English.
  base::i18n::SetICUDefaultLocale("en");
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      centerAlignedTarget_, bubbleSize_, BubbleArrowDirectionDown,
      BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(kTestBubbleAlignmentOffset, bubbleFrame.origin.x);
  EXPECT_EQ(100.0f, bubbleFrame.origin.y);
  EXPECT_EQ(bubbleSize_.width, bubbleFrame.size.width);
  EXPECT_EQ(bubbleSize_.height, bubbleFrame.size.height);
}

// Test bubble_util::BubbleFrame when the bubble's direction is
// |BubbleArrowDirectionDown|, the alignment is |BubbleAlignmentTrailing|, and
// the language is RTL
TEST_F(BubbleUtilTest, BubbleFrameDownTrailingRTL) {
  // Ensure that the language is RTL by setting the locale to Hebrew.
  base::i18n::SetICUDefaultLocale("he");
  CGRect bubbleFrame = bubble_util::BubbleFrame(
      centerAlignedTarget_, bubbleSize_, BubbleArrowDirectionDown,
      BubbleAlignmentTrailing, containerWidth_);
  EXPECT_EQ(200.0f - kTestBubbleAlignmentOffset, bubbleFrame.origin.x);
  EXPECT_EQ(100.0f, bubbleFrame.origin.y);
  EXPECT_EQ(bubbleSize_.width, bubbleFrame.size.width);
  EXPECT_EQ(bubbleSize_.height, bubbleFrame.size.height);
}
