// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_EXTERNAL_APP_EXTERNAL_APP_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_EXTERNAL_APP_EXTERNAL_APP_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/web/external_app_presenter.h"

// A coordinator that handles UI related to launching external apps.
@interface ExternalAppCoordinator : NSObject<ExternalAppPresenter>
// The base view controller from which to present UI.
@property(nonatomic, weak) UIViewController* baseViewController;

// Initializers.
- (instancetype)initWithBaseViewController:(UIViewController*)baseViewController
    NS_DESIGNATED_INITIALIZER;
- (instancetype)init NS_UNAVAILABLE;
@end

#endif  // IOS_CHROME_BROWSER_UI_EXTERNAL_APP_EXTERNAL_APP_COORDINATOR_H_
