// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_util.h"

#include "base/i18n/rtl.h"
#import "ios/chrome/browser/ui/bubble/bubble_view.h"

namespace bubble_util {

CGPoint Origin(CGRect elementFrame,
               BubbleAlignment alignment,
               BubbleArrowDirection arrowDirection,
               CGSize size) {
  // Find |originOffset|, the distance from the bubble's left edge to the
  // anchor point. This depends on alignment, whether the language is RTL, and
  // bubble width.
  CGPoint anchorPoint = AnchorPoint(elementFrame, arrowDirection);
  CGFloat originOffset;
  switch (alignment) {
    case BubbleAlignmentLeading:
      if (base::i18n::IsRTL()) {
        originOffset = size.width - BubbleAlignmentOffset;
      } else {
        originOffset = BubbleAlignmentOffset;
      }
      break;
    case BubbleAlignmentCenter:
      originOffset = size.width / 2.0f;
    case BubbleAlignmentTrailing:
      if (base::i18n::IsRTL()) {
        originOffset = BubbleAlignmentOffset;
      } else {
        originOffset = size.width - BubbleAlignmentOffset;
      }
    default:
      break;
  }

  CGPoint bubbleOrigin;
  bubbleOrigin.x = anchorPoint.x - originOffset;
  if (arrowDirection == BubbleArrowDirectionUp) {
    bubbleOrigin.y = anchorPoint.y;
  } else {
    bubbleOrigin.y = anchorPoint.y - size.height;
  }

  return bubbleOrigin;
}

CGFloat MaxWidth(CGRect elementFrame,
                 BubbleAlignment alignment,
                 CGFloat superviewWidth) {
  CGFloat anchorX = CGRectGetMidX(elementFrame);
  CGFloat maxWidth;
  switch (alignment) {
    case BubbleAlignmentLeading:
      if (base::i18n::IsRTL()) {
        maxWidth = anchorX + BubbleAlignmentOffset;
      } else {
        maxWidth = superviewWidth - anchorX + BubbleAlignmentOffset;
      }
      break;
    case BubbleAlignmentCenter:
      maxWidth = MIN(anchorX, superviewWidth - anchorX) * 2.0f;
    case BubbleAlignmentTrailing:
      if (base::i18n::IsRTL()) {
        maxWidth = superviewWidth - anchorX + BubbleAlignmentOffset;
      } else {
        maxWidth = anchorX + BubbleAlignmentOffset;
      }
    default:
      break;
  }
  return maxWidth;
}

// Calculate the coordinates of the point of the speech bubble's triangle based
// on the |frame| of the target UI element and the bubble's |arrowDirection|.
// The returned point is in the same coordinate system as |frame|.
CGPoint AnchorPoint(CGRect elementFrame, BubbleArrowDirection arrowDirection) {
  CGPoint anchorPoint;
  anchorPoint.x = CGRectGetMidX(elementFrame);
  if (arrowDirection == BubbleArrowDirectionUp) {
    anchorPoint.y = CGRectGetMaxY(elementFrame);
  } else {
    anchorPoint.y = CGRectGetMinY(elementFrame);
  }
  return anchorPoint;
}

}  // namespace bubble_util
