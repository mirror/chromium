// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_PRIMARY_TOOLBAR_VIEW_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_PRIMARY_TOOLBAR_VIEW_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/toolbar/adaptive/adaptive_toolbar_view.h"

@class ToolbarButtonFactory;

// View for the primary toolbar. In an adaptive toolbar paradigm, this is the
// toolbar always displayed.
@interface PrimaryToolbarView : UIView<AdaptiveToolbarView>

// Initialize this View with the button |factory| and the |topSafeAnchor|.
- (instancetype)initWithButtonFactory:(ToolbarButtonFactory*)factory
                        topSafeAnchor:(NSLayoutYAxisAnchor*)topSafeAnchor
    NS_DESIGNATED_INITIALIZER;

// Initialize this View with the button |factory| and use the safe layout guide
// for the |topSafeAnchor| of the designated initializer.
- (instancetype)initWithButtonFactory:(ToolbarButtonFactory*)factory
    API_AVAILABLE(ios(11.0));

- (instancetype)initWithFrame:(CGRect)frame NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder*)aDecoder NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;

// The location bar view, containing the omnibox.
@property(nonatomic, strong) UIView* locationBarView;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_PRIMARY_TOOLBAR_VIEW_H_
