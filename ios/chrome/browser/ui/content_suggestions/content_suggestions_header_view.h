// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_VIEW_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_VIEW_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/ntp/ntp_header_adapter.h"
#import "ios/chrome/browser/ui/toolbar/toolbar_owner.h"

// Header view for the Material Design NTP. The header view contains all views
// that are displayed above the list of most visited sites, which includes the
// toolbar buttons, Google doodle, and fake omnibox.
@interface ContentSuggestionsHeaderView
    : UIView<ToolbarOwner, NTPHeaderViewAdapter>

// Return the toolbar view;
@property(nonatomic, readonly) UIView* toolBarView;

// Add toolbar to view.
- (void)addToolbarView:(UIView*)toolbarView;

// Changes the constraints of searchField based on its initialFrame and the
// scroll view's y |offset|. Also adjust the alpha values for |_searchBoxBorder|
// and |_shadow| and the constant values for the |constraints|.|screenWidth| is
// the width of the screen, including the space outside the safe area. The
// |safeAreaInsets| is relative to the view used to calculate the |width|.
- (void)updateSearchFieldWidth:(NSLayoutConstraint*)widthConstraint
                        height:(NSLayoutConstraint*)heightConstraint
                     topMargin:(NSLayoutConstraint*)topMarginConstraint
            subviewConstraints:(NSArray*)constraints
                     forOffset:(CGFloat)offset
                   screenWidth:(CGFloat)screenWidth
                safeAreaInsets:(UIEdgeInsets)safeAreaInsets;

// Initializes |_searchBoxBorder| and |_shadow| and adds them to |searchField|.
- (void)addViewsToSearchField:(UIView*)searchField;

@end

#endif  // IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_VIEW_H_
