// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_util.h"

#include "base/i18n/rtl.h"

CGFloat BubbleAlignmentOffset = 10.0f;

CGPoint BubbleAnchorPoint(CGRect frame, BubbleArrowDirection arrowDirection) {
  CGPoint anchorPoint;
  anchorPoint.x = CGRectGetMidX(frame);
  if (arrowDirection == BubbleArrowDirectionUp) {
    anchorPoint.y = CGRectGetMaxY(frame);
  } else {
    anchorPoint.y = CGRectGetMinY(frame);
  }
  return anchorPoint;
}

CGPoint BubbleOrigin(CGPoint anchorPoint,
                     BubbleAlignment alignment,
                     BubbleArrowDirection arrowDirection,
                     CGSize size) {
  // Find |leadingOffset|, the distance from the bubble's leading edge to the
  // anchor point. This depends on alignment, whether the language is RTL, and
  // bubble width.
  CGFloat leadingOffset;
  switch (alignment) {
    case BubbleAlignmentLeading:
      if (base::i18n::IsRTL()) {
        leadingOffset = size.width - BubbleAlignmentOffset;
      } else {
        leadingOffset = BubbleAlignmentOffset;
      }
      break;
    case BubbleAlignmentCenter:
      leadingOffset = size.width / 2.0f;
    case BubbleAlignmentTrailing:
      if (base::i18n::IsRTL()) {
        leadingOffset = BubbleAlignmentOffset;
      } else {
        leadingOffset = size.width - BubbleAlignmentOffset;
      }
    default:
      break;
  }

  CGPoint bubbleOrigin;
  bubbleOrigin.x = anchorPoint.x - leadingOffset;
  if (arrowDirection == BubbleArrowDirectionUp) {
    bubbleOrigin.y = anchorPoint.y;
  } else {
    bubbleOrigin.y = anchorPoint.y - size.height;
  }

  return bubbleOrigin;
}
