// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_
#define IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_

#import "ios/chrome/browser/ui/bubble/bubble_view.h"

namespace bubble_util {

// Calculate the coordinates of the point of the speech bubble's triangle based
// on the |frame| of the target UI element and the bubble's |arrowDirection|.
// The returned point is in the same coordinate system as |frame|.
CGPoint AnchorPoint(CGRect targetFrame, BubbleArrowDirection arrowDirection);

// Calculate the maximum width of the bubble such that it stays within its
// bounding coordinate space. |anchorPoint| is the point of the target UI
// element at which the bubble is anchored. It is in the coordinate system in
// which the bubble is drawn. |alignment| is the bubble's alignment, and
// |boundingWidth| is the width of the coordinate space in which the bubble is
// drawn.
CGFloat MaxWidth(CGPoint anchorPoint,
                 BubbleAlignment alignment,
                 CGFloat boundingWidth);

// Calculate the bubble's frame. |targetFrame| is the frame of the UI element
// the bubble is pointing to. |size| is the size of the bubble. |direction| is
// the direction the bubble's arrow is pointing. |alignment| is the alignment of
// the anchor (either leading, centered, or trailing). |boundingWidth| is the
// width of the bubble's superview.
CGRect BubbleFrame(CGPoint anchorPoint,
                   CGSize size,
                   BubbleArrowDirection direction,
                   BubbleAlignment alignment,
                   CGFloat boundingWidth);

// Returns the portion of |rect| that can enclose a bubble pointed anchored at
// |anchorPoint| and pointing in |direction|. If |direction| is
// |BubbleViewArrowDirectionUp|, then the rectangle inside of |rect| and beneath
// |anchorPoint| is returned. If |direction| is |BubbleViewArrowDirectionDown|,
// then the rectangle inside of |rect| and above |anchorPoint| is returned.
CGRect DivideRectAtPoint(CGRect rect,
                         CGPoint anchorPoint,
                         BubbleArrowDirection direction);

}  // namespace bubble_util

#endif  // IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_
