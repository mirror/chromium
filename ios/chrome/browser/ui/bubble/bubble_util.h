// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_
#define IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_

#import "ios/chrome/browser/ui/bubble/bubble_view.h"

// Calculate the coordinates of the point of the speech bubble's triangle based
// on |frame|, the frame of the target UI element, and |arrowDirection|.
CGPoint BubbleAnchorPoint(CGRect frame, BubbleArrowDirection arrowDirection);

// Calculate the bubble's origin. Alignment is determined using |maxFrame|, the
// frame that the bubble must stay within.
CGPoint BubbleOrigin(CGPoint anchorPoint,
                     BubbleArrowDirection arrowDirection,
                     CGSize size,
                     CGRect maxFrame);

#endif  // IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_UTIL_H_
