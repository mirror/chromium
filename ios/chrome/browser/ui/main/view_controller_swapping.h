// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_MAIN_VIEW_CONTROLLER_SWAPPING_H_
#define IOS_CHROME_BROWSER_UI_MAIN_VIEW_CONTROLLER_SWAPPING_H_

#import <UIKit/UIKit.h>

#import "base/ios/block_types.h"

// ViewControllerSwapping defines a set of methods that allow an object to
// display TabSwitchers and BrowserViewControllers.
@protocol ViewControllerSwapping

// The view controller, if any, that is active.
@property(nonatomic, readonly, strong) UIViewController* activeViewController;

// Swaps in and displays the given TabSwitcher, replacing any other TabSwitchers
// or BrowserViewControllers that may currently be visible.  Runs the given
// |completion| block after the view controller is visible.
- (void)showTabSwitcher:(UIViewController*)viewController
             completion:(ProceduralBlock)completion;

// Swaps in and displays the given BrowserViewController, replacing any other
// TabSwitchers or BrowserViewControllers that may currently be visible.  Runs
// the given |completion| block after the view controller is visible.
- (void)showBrowserViewController:(UIViewController*)viewController
                       completion:(ProceduralBlock)completion;

@end

#endif  // IOS_CHROME_BROWSER_UI_MAIN_VIEW_CONTROLLER_SWAPPING_H_
