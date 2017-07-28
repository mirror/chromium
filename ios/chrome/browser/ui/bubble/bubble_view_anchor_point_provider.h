// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_VIEW_ANCHOR_POINT_PROVIDER_H_
#define IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_VIEW_ANCHOR_POINT_PROVIDER_H_

#import "ios/chrome/browser/ui/bubble/bubble_view.h"

// Defines an interface for accessing the anchor points of toolbar elements.
// Points are expected to be in window-coordinates.
@protocol BubbleViewAnchorPointProvider

// Returns either the top-middle or bottom-middle of the tab switcher button
// based on |direction|.
- (CGPoint)anchorPointForTabSwitcherButton:(BubbleArrowDirection)direction;

// Returns either the top-middle or bottom-middle of the tools menu button
// based on |direction|.
- (CGPoint)anchorPointForToolsMenuButton:(BubbleArrowDirection)direction;

@end

#endif  // IOS_CHROME_BROWSER_UI_BUBBLE_BUBBLE_VIEW_ANCHOR_POINT_PROVIDER_H_
