// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_VIEW_H_
#define IOS_CHROME_BROWSER_UI_CONTENT_SUGGESTIONS_CONTENT_SUGGESTIONS_HEADER_VIEW_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/toolbar/toolbar_owner.h"

// Temporary ContentSuggestionsHeaderViewAdapter to simplify deprecating
// NewTabPageHeaderView.
class ReadingListModel;

@protocol ContentSuggestionsHeaderViewAdapter<NSObject, ToolbarOwner>

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

// Header view for the Material Design NTP. The header view contains all views
// that are displayed above the list of most visited sites, which includes the
// toolbar buttons, Google doodle, and fake omnibox.
@interface ContentSuggestionsHeaderView
    : UIView<ToolbarOwner, ContentSuggestionsHeaderViewAdapter>

// Return the toolbar view;
@property(nonatomic, readonly) UIView* toolBarView;

// Foo.
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
