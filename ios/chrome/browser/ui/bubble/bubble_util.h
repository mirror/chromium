// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_
#define IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_

#import "ios/chrome/browser/ui/bubble/bubble_view.h"

// The fixed distance from the leading edge of the bubble to the anchor point if
// aligned leading, and from the trailing edge of the bubble to the anchor point
// if aligned trailing.
extern CGFloat alignmentOffset;

// Calculate the coordinates of the point of the speech bubble's triangle based
// on the frame of the target UI element and the bubble's arrow direction. The
// returned point is in the same coordinate system as |frame|.
CGPoint BubbleAnchorPoint(CGRect frame, BubbleArrowDirection arrowDirection);

// Calculate the bubble's origin based on the given anchor point and the
// the bubble's alignment, direction, and size. The returned point is in the
// same coordinate system as |anchorPoint|, which should be the bubble's
// superview's coordinate system.
CGPoint BubbleOrigin(CGPoint anchorPoint,
                     BubbleAlignment alignment,
                     BubbleArrowDirection arrowDirection,
                     CGSize size);

#endif  // IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_
