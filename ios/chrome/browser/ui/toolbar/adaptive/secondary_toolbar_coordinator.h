// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_SECONDARY_TOOLBAR_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_SECONDARY_TOOLBAR_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"

@protocol ApplicationCommands;
@protocol BrowserCommands;

// Coordinator for the secondary toolbar.
@interface SecondaryToolbarCoordinator : ChromeCoordinator

- (nullable instancetype)initWithBaseViewController:
    (nullable UIViewController*)viewController NS_UNAVAILABLE;

// UIViewController managed by this coordinator;
@property(nonatomic, strong, readonly, nullable)
    UIViewController* viewController;
// Dispatcher.
@property(nonatomic, weak, nullable) id<ApplicationCommands, BrowserCommands>
    dispatcher;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_SECONDARY_TOOLBAR_COORDINATOR_H_
