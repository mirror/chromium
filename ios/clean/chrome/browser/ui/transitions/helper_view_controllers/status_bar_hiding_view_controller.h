// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_TRANSITIONS_HELPER_VIEW_CONTROLLER_STATUS_BAR_HIDING_VIEW_CONTROLLER_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_TRANSITIONS_HELPER_VIEW_CONTROLLER_STATUS_BAR_HIDING_VIEW_CONTROLLER_H_

#import <UIKit/UIKit.h>

// This is a helper view controller that will add a small view with backgound
// color |color| at the top of the view.
// When animating the ViewController in from the top of the screen, this will
// set the color of the status bar.
@interface StatusBarHidingViewController : UIViewController
- (instancetype)initWithChildViewController:(UIViewController*)child
                             statusBarColor:(UIColor*)color;
@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_TRANSITIONS_HELPER_VIEW_CONTROLLER_STATUS_BAR_HIDING_VIEW_CONTROLLER_H_
