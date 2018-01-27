// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_NTP_NTP_HEADER_ADAPTER_H_
#define IOS_CHROME_BROWSER_UI_NTP_NTP_HEADER_ADAPTER_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/toolbar/toolbar_owner.h"

// crbug.com/XXX Remove this protocol once the ui refresh complete.
class ReadingListModel;

@protocol NTPHeaderViewAdapter<NSObject, ToolbarOwner>

@property(nonatomic, readonly) UIView* toolBarView;

- (void)updateSearchFieldWidth:(NSLayoutConstraint*)widthConstraint
                        height:(NSLayoutConstraint*)heightConstraint
                     topMargin:(NSLayoutConstraint*)topMarginConstraint
            subviewConstraints:(NSArray*)constraints
                     forOffset:(CGFloat)offset
                   screenWidth:(CGFloat)screenWidth
                safeAreaInsets:(UIEdgeInsets)safeAreaInsets;

// Initializes |_searchBoxBorder| and |_shadow| and adds them to |searchField|.
- (void)addViewsToSearchField:(UIView*)searchField;

// Animates |_shadow|'s alpha to 0.
- (void)fadeOutShadow;

// Hide toolbar subviews that should not be displayed on the new tab page.
- (void)hideToolbarViewsForNewTabPage;

// Updates the toolbar tab count;
- (void)setToolbarTabCount:(int)tabCount;

// |YES| if the toolbar can show the forward arrow.
- (void)setCanGoForward:(BOOL)canGoForward;

// |YES| if the toolbar can show the back arrow.
- (void)setCanGoBack:(BOOL)canGoBack;

@optional
- (void)addToolbarView:(UIView*)toolbarView;
- (void)addToolbarWithReadingListModel:(ReadingListModel*)readingListModel
                            dispatcher:(id)dispatcher;
@end

#endif  // IOS_CHROME_BROWSER_UI_NTP_NTP_HEADER_ADAPTER_H_
