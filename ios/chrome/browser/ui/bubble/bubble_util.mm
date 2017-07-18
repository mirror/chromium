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
  // Depending on alignment and bubble width, find |leadingOffset|, the
  // distance from the bubble's leading edge to the anchor point.
  CGFloat leadingOffset;
  switch (alignment) {
    case BubbleAlignmentLeading:
      leadingOffset = BubbleAlignmentOffset;
      break;
    case BubbleAlignmentCenter:
      leadingOffset = size.width / 2.0f;
    case BubbleAlignmentTrailing:
      leadingOffset = size.width - BubbleAlignmentOffset;
    default:
      break;
  }

  CGPoint bubbleOrigin;
  // Depending on |leadingOffset| and whether LTR/RTL, set |bubbleOrigin.x|.
  if (base::i18n::IsRTL()) {
    bubbleOrigin.x = anchorPoint.x + leadingOffset;
  } else {
    bubbleOrigin.x = anchorPoint.x - leadingOffset;
  }
  if (arrowDirection == BubbleArrowDirectionUp) {
    bubbleOrigin.y = anchorPoint.y;
  } else {
    bubbleOrigin.y = anchorPoint.y - size.height;
  }

  return bubbleOrigin;
}
