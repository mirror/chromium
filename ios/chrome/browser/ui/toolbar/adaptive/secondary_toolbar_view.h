// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_SECONDARY_TOOLBAR_VIEW_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_SECONDARY_TOOLBAR_VIEW_H_

#import <UIKit/UIKit.h>

@class ToolbarButton;
@class ToolbarButtonFactory;
@class ToolbarToolsMenuButton;

// View for the secondary toolbar.
@interface SecondaryToolbarView : UIView

// Anchor at the top of the bottom safeAreaLayoutGuide. Used so views don't go
// below the safe area.
@property(nonatomic, strong) NSLayoutYAxisAnchor* bottomSafeAnchor;
// Factory used to create the buttons.
@property(nonatomic, strong) ToolbarButtonFactory* buttonFactory;

// Buttons
@property(nonatomic, strong, readonly) ToolbarToolsMenuButton* toolsMenuButton;
@property(nonatomic, strong, readonly) ToolbarButton* tabGridButton;
@property(nonatomic, strong, readonly) ToolbarButton* shareButton;
@property(nonatomic, strong, readonly) ToolbarButton* omniboxButton;
@property(nonatomic, strong, readonly) ToolbarButton* bookmarksButton;

// Sets all the subviews.
- (void)setUp;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_SECONDARY_TOOLBAR_VIEW_H_
