// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/bubble/bubble_util.h"
#include "base/i18n/rtl.h"

CGPoint BubbleAnchorPoint(CGRect frame, BubbleArrowDirection arrowDirection) {
  CGPoint anchorPoint;
  anchorPoint.x = frame.origin.x + frame.size.width / 2.0f;
  if (arrowDirection == BubbleArrowDirectionUp) {
    anchorPoint.y = frame.origin.y + frame.size.height;
  } else {
    anchorPoint.y = frame.origin.y;
  }
  return anchorPoint;
}

CGPoint BubbleOrigin(CGPoint anchorPoint,
                     BubbleArrowDirection arrowDirection,
                     CGSize size,
                     CGRect maxFrame) {
  CGPoint bubbleOrigin;
  if (anchorPoint.x - size.width < maxFrame.origin.x) {
    // If |anchorPoint| is too far left to center align the bubble, align it to
    // the left edge of |maxFrame|.
    bubbleOrigin.x = maxFrame.origin.x;
  } else if (anchorPoint.x + size.width >
             maxFrame.origin.x + maxFrame.size.width) {
    // If |anchorPoint| is too far right to center align the bubble, align it to
    // the right edge of |maxFrame|.
    bubbleOrigin.x = maxFrame.origin.x + maxFrame.size.width - size.width;
  } else {
    // Center align to |anchorPoint| if the bubble would be within |maxFrame|.
    bubbleOrigin.x = anchorPoint.x - size.width / 2.0f;
  }
  if (arrowDirection == BubbleArrowDirectionUp) {
    bubbleOrigin.y = anchorPoint.y;
  } else {
    bubbleOrigin.y = anchorPoint.y - size.height;
  }

  return bubbleOrigin;
}
