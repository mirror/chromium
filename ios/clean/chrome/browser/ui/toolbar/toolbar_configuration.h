// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_CONFIGURATION_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_CONFIGURATION_H_

#import <UIKit/UIKit.h>

// Toolbar configuration protocol giving access to styling elements. The style
// depends of the implementation.
@protocol ToolbarConfiguration

// Background color of the toolbar.
- (UIColor*)backgroundColor;

// Background color of the omnibox.
- (UIColor*)omniboxBackgroundColor;

// Border color of the omnibox.
- (UIColor*)omniboxBorderColor;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_CONFIGURATION_H_
