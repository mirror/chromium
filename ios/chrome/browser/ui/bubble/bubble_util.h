// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_
#define IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_

#import "ios/chrome/browser/ui/bubble/bubble_view.h"

namespace bubble_util {

// Calculate the bubble's origin based on |elementFrame|, which is the frame of
// the target UI element, and the bubble's alignment, direction, and size. The
// returned point is in the same coordinate system as |elementFrame|, which
// should be the coordinate system in which the bubble is drawn.
CGFloat LeadingDistance(CGRect elementFrame,
                        BubbleAlignment alignment,
                        CGFloat bubbleWidth);

CGFloat OriginY(CGRect elementFrame,
                BubbleArrowDirection arrowDirection,
                CGFloat bubbleHeight);

// Calculate the maximum width of the bubble such that it stays within its
// bounding coordinate space. |elementFrame| is the frame the target UI element
// in the coordinate system in which the bubble is drawn. |alignment| is the
// bubble's alignment, and |boundingWidth| is the width of the coordinate space
// in which the bubble is drawn.
CGFloat MaxWidth(CGRect elementFrame,
                 BubbleAlignment alignment,
                 CGFloat boundingWidth);

}  // namespace bubble_util

#endif  // IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_
