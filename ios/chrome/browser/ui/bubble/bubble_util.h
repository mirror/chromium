// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_
#define IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_

#import "ios/chrome/browser/ui/bubble/bubble_view.h"

namespace bubble_util {

// Calculate the maximum width of the bubble such that it stays within its
// bounding coordinate space. |targetFrame| is the frame the target UI element
// in the coordinate system in which the bubble is drawn. |alignment| is the
// bubble's alignment, and |boundingWidth| is the width of the coordinate space
// in which the bubble is drawn.
CGFloat MaxWidth(CGRect targetFrame,
                 BubbleAlignment alignment,
                 CGFloat boundingWidth);

// Calculate the bubble's frame. |targetFrame| is the frame of the UI element
// the bubble is pointing to. |size| is the size of the bubble. |direction| is
// the direction the bubble's arrow is pointing. |alignment| is the alignment of
// the anchor (either leading, centered, or trailing). |boundingWidth| is the
// width of the bubble's superview.
CGRect BubbleFrame(CGRect targetFrame,
                   CGSize size,
                   BubbleArrowDirection direction,
                   BubbleAlignment alignment,
                   CGFloat boundingWidth);

}  // namespace bubble_util

#endif  // IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_
