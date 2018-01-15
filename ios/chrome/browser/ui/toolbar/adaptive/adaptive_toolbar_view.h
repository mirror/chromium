// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_ADAPTIVE_TOOLBAR_VIEW_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_ADAPTIVE_TOOLBAR_VIEW_H_

#import <UIKit/UIKit.h>

@class MDCProgressView;
@class ToolbarButton;
@class ToolbarButtonFactory;
@class ToolbarToolsMenuButton;

// Protocol defining the interface for interacting with a view of the adaptive
// toolbar.
@protocol AdaptiveToolbarView<NSObject>

// Top anchor at the bottom of the safeAreaLayoutGuide. Used so views don't
// overlap with the Status Bar.
@property(nonatomic, strong) NSLayoutYAxisAnchor* topSafeAnchor;
// Factory used to create the buttons.
@property(nonatomic, strong) ToolbarButtonFactory* buttonFactory;
// The location bar view, containing the omnibox.
@property(nonatomic, strong) UIView* locationBarView;

// Property to get all the buttons in this view.
@property(nonatomic, strong, readonly) NSArray<ToolbarButton*>* allButtons;

// Progress bar displayed below the toolbar.
@property(nonatomic, strong, readonly) MDCProgressView* progressBar;

// Button to navigate back.
@property(nonatomic, strong, readonly) ToolbarButton* backButton;
// Button to navigate forward, leading position.
@property(nonatomic, strong, readonly) ToolbarButton* forwardLeadingButton;
// Button to display the TabGrid.
@property(nonatomic, strong, readonly) ToolbarButton* tabGridButton;
// Button to stop the loading of the page.
@property(nonatomic, strong, readonly) ToolbarButton* stopButton;
// Button to reload the page.
@property(nonatomic, strong, readonly) ToolbarButton* reloadButton;
// Button to navigate forward, trailing position.
@property(nonatomic, strong, readonly) ToolbarButton* forwardTrailingButton;
// Button to display the share menu.
@property(nonatomic, strong, readonly) ToolbarButton* shareButton;
// Button to manage the bookmarks of this page.
@property(nonatomic, strong, readonly) ToolbarButton* bookmarkButton;
// Button to display the tools menu.
@property(nonatomic, strong, readonly) ToolbarToolsMenuButton* toolsMenuButton;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_ADAPTIVE_TOOLBAR_VIEW_H_
