// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_APP_LAUNCHER_APP_LAUNCHER_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_APP_LAUNCHER_APP_LAUNCHER_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/web/app_launcher_presenter.h"

// A coordinator that handles UI related to launching apps.
@interface AppLauncherCoordinator : NSObject<AppLauncherPresenter>

// The base view controller from which to present UI.
@property(nonatomic, weak) UIViewController* baseViewController;

// Initializes the coordinator with the |baseViewController|, from which to
// present UI.
- (instancetype)initWithBaseViewController:(UIViewController*)baseViewController
    NS_DESIGNATED_INITIALIZER;

// Default designated initializer is unavailable.
- (instancetype)init NS_UNAVAILABLE;

@end

#endif  // IOS_CHROME_BROWSER_UI_APP_LAUNCHER_APP_LAUNCHER_COORDINATOR_H_
