// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_
#define IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_

#import "ios/chrome/browser/ui/bubble/bubble_view.h"

namespace bubble_util {

// Calculate the bubble's origin based on the given anchor point and the
// the bubble's alignment, direction, and size. The returned point is in the
// same coordinate system as |anchorPoint|, which should be the bubble's
// superview's coordinate system.
CGPoint Origin(CGRect elementFrame,
               BubbleAlignment alignment,
               BubbleArrowDirection arrowDirection,
               CGSize size);

// Calculate the maximum width of the bubble such that it stays within the
// superview's bounds. Determined based on the target UI element, the bubble's
// alignment, and the width of the bubble's superview. |elementFrame| should be
// in the coordinate system of the bubble's superview.
CGFloat MaxWidth(CGRect elementFrame,
                 BubbleAlignment alignment,
                 CGFloat superviewWidth);
}  // namespace bubble_util

#endif  // IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_
