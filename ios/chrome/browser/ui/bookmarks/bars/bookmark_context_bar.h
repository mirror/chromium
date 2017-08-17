// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_BOOKMARKS_BARS_BOOKMARK_CONTEXT_BAR_H_
#define IOS_CHROME_BROWSER_UI_BOOKMARKS_BARS_BOOKMARK_CONTEXT_BAR_H_

#import <UIKit/UIKit.h>

// Delegate for BookmarkContextBar. Used to pass on button actions.
@protocol ContextBarDelegate<NSObject>
// Called when the leading button is clicked.
- (void)leadingButtonClicked;
// Called when the center button is clicked.
- (void)centerButtonClicked;
// Called when the trailing button is clicked.
- (void)trailingButtonClicked;
@end

typedef NS_ENUM(NSInteger, ContextBarButton) {
  ContextBarLeadingButton,
  ContextBarCenterButton,
  ContextBarTrailingButton
};

typedef NS_ENUM(NSInteger, ContextBarButtonStyle) {
  ContextBarButtonStyleNone,
  ContextBarButtonStyleDelete
};

// View at the bottom of the bookmark panel that presents options "New Folder"
// and "Select" when browsing bookmark; and presents options "Delete", "More..."
// and "Cancel" when selecting bookmarks.
@interface BookmarkContextBar : UIView

@property(nonatomic, weak) id<ContextBarDelegate> delegate;

- (void)setButtonTitle:(NSString*)title forButton:(ContextBarButton)button;
- (void)setButtonStyle:(ContextBarButtonStyle)style
             forButton:(ContextBarButton)button;
- (void)setButtonVisibilityButton:(BOOL)visible
                        forButton:(ContextBarButton)button;

@end
#endif  // IOS_CHROME_BROWSER_UI_BOOKMARKS_BARS_BOOKMARK_CONTEXT_BAR_H_
