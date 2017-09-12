// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_CONFIGURATION_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_CONFIGURATION_H_

#import <UIKit/UIKit.h>

#import "ios/clean/chrome/browser/ui/toolbar/toolbar_style.h"

// Toolbar configuration object giving access to styling elements.
@interface ToolbarConfiguration : NSObject

- (instancetype)initWithStyle:(ToolbarStyle)style NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;

@property(nonatomic, assign) ToolbarStyle style;

// Background color of the toolbar.
- (UIColor*)backgroundColor;

// Background color of the omnibox.
- (UIColor*)omniboxBackgroundColor;

// Border color of the omnibox.
- (UIColor*)omniboxBorderColor;

- (UIColor*)buttonTitleNormalColor;

- (UIColor*)buttonTitleHighlightedColor;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_CONFIGURATION_H_
