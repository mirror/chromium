// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_util.h"

#include "base/i18n/rtl.h"
#include "base/logging.h"
#import "ios/chrome/browser/ui/bubble/bubble_view.h"
#import "ios/chrome/browser/ui/rtl_geometry.h"

namespace bubble_util {

namespace {
// Calculate the distance from the bubble's leading edge to the leading edge of
// its bounding coordinate system. In LTR contexts, the returned float is the
// x-coordinate of the bubble's origin. This calculation is based on
// |targetFrame|, which is the frame of the target UI element, and the bubble's
// alignment, direction, and size. The returned float is in the same coordinate
// system as |targetFrame|, which should be the coordinate system in which the
// bubble is drawn.
CGFloat LeadingDistance(CGPoint anchorPoint,
                        BubbleAlignment alignment,
                        CGFloat bubbleWidth) {
  // Find |leadingOffset|, the distance from the bubble's leading edge to the
  // anchor point. This depends on alignment and bubble width.
  CGFloat anchorX = anchorPoint.x;
  CGFloat leadingOffset;
  switch (alignment) {
    case BubbleAlignmentLeading:
      leadingOffset = kBubbleAlignmentOffset;
      break;
    case BubbleAlignmentCenter:
      leadingOffset = bubbleWidth / 2.0f;
      break;
    case BubbleAlignmentTrailing:
      leadingOffset = bubbleWidth - kBubbleAlignmentOffset;
      break;
    default:
      NOTREACHED() << "Invalid bubble alignment " << alignment;
      break;
  }
  return anchorX - leadingOffset;
}

// Calculate the y-coordinate of the bubble's origin based on |targetFrame|, and
// the bubble's arrow direction and size. The returned float is in the same
// coordinate system as |targetFrame|, which should be the coordinate system in
// which the bubble is drawn.
CGFloat OriginY(CGPoint anchorPoint,
                BubbleArrowDirection arrowDirection,
                CGFloat bubbleHeight) {
  CGFloat originY;
  if (arrowDirection == BubbleArrowDirectionUp) {
    originY = anchorPoint.y;
  } else {
    DCHECK_EQ(arrowDirection, BubbleArrowDirectionDown);
    originY = anchorPoint.y - bubbleHeight;
  }
  return originY;
}
}  // namespace

CGPoint AnchorPoint(CGRect targetFrame, BubbleArrowDirection arrowDirection) {
  CGPoint anchorPoint;
  anchorPoint.x = CGRectGetMidX(targetFrame);
  if (arrowDirection == BubbleArrowDirectionUp) {
    anchorPoint.y = CGRectGetMaxY(targetFrame);
  } else {
    DCHECK_EQ(arrowDirection, BubbleArrowDirectionDown);
    anchorPoint.y = CGRectGetMinY(targetFrame);
  }
  return anchorPoint;
}

CGFloat MaxWidth(CGPoint anchorPoint,
                 BubbleAlignment alignment,
                 CGFloat boundingWidth) {
  CGFloat anchorX = anchorPoint.x;
  CGFloat maxWidth;
  switch (alignment) {
    case BubbleAlignmentLeading:
      if (base::i18n::IsRTL()) {
        // The bubble is aligned right, and can use space to the left of the
        // anchor point and within |kBubbleAlignmentOffset| from the right.
        maxWidth = anchorX + kBubbleAlignmentOffset;
      } else {
        // The bubble is aligned left, and can use space to the right of the
        // anchor point and within |kBubbleAlignmentOffset| from the left.
        maxWidth = boundingWidth - anchorX + kBubbleAlignmentOffset;
      }
      break;
    case BubbleAlignmentCenter:
      // The width of half the bubble cannot exceed the distance from the anchor
      // point to the closest edge of the superview.
      maxWidth = MIN(anchorX, boundingWidth - anchorX) * 2.0f;
      break;
    case BubbleAlignmentTrailing:
      if (base::i18n::IsRTL()) {
        // The bubble is aligned left, and can use space to the right of the
        // anchor point and within |kBubbleAlignmentOffset| from the left.
        maxWidth = boundingWidth - anchorX + kBubbleAlignmentOffset;
      } else {
        // The bubble is aligned right, and can use space to the left of the
        // anchor point and within |kBubbleAlignmentOffset| from the right.
        maxWidth = anchorX + kBubbleAlignmentOffset;
      }
      break;
    default:
      NOTREACHED() << "Invalid bubble alignment " << alignment;
      break;
  }
  return maxWidth;
}

CGRect BubbleFrame(CGPoint anchorPoint,
                   CGSize size,
                   BubbleArrowDirection direction,
                   BubbleAlignment alignment,
                   CGFloat boundingWidth) {
  CGFloat leading =
      bubble_util::LeadingDistance(anchorPoint, alignment, size.width);
  CGFloat originY = bubble_util::OriginY(anchorPoint, direction, size.height);
  // Use a |LayoutRect| to ensure that the bubble is mirrored in RTL contexts.
  CGRect bubbleFrame = LayoutRectGetRect(
      LayoutRectMake(leading, boundingWidth, originY, size.width, size.height));
  return bubbleFrame;
}

}  // namespace bubble_util
