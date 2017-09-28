// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_COORDINATOR_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/browser_coordinator.h"

@class MainContentViewController;

// A coordinator that displays the main content view of the browser.  This
// should be subclassed by any coordinator that displays the scrollable content
// related to the current URL.
@interface MainContentCoordinator : BrowserCoordinator

// The view controller used to display the main content view.
@property(nonatomic, readonly) MainContentViewController* viewController;

@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_MAIN_CONTENT_MAIN_CONTENT_COORDINATOR_H_
