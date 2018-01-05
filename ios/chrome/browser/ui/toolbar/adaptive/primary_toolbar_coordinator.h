// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_PRIMARY_TOOLBAR_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_PRIMARY_TOOLBAR_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/coordinators/chrome_coordinator.h"
#import "ios/chrome/browser/ui/toolbar/public/primary_toolbar_coordinator.h"

@protocol ApplicationCommands;
@protocol BrowserCommands;
@class CommandDispatcher;
@protocol ToolbarCoordinatorDelegate;
@protocol ToolsMenuConfigurationProvider;
@protocol UrlLoader;
class WebStateList;

// Coordinator for the primary toolbar. In an adaptive toolbar paradigm, this is
// the toolbar always presented.
@interface PrimaryToolbarCoordinator
    : ChromeCoordinator<PrimaryToolbarCoordinator>

- (nullable instancetype)
initWithToolsMenuConfigurationProvider:
    (nonnull id<ToolsMenuConfigurationProvider>)configurationProvider
                            dispatcher:(nonnull CommandDispatcher*)dispatcher
                          browserState:
                              (ios::ChromeBrowserState* _Nonnull)browserState
    NS_DESIGNATED_INITIALIZER;
- (nullable instancetype)initWithBaseViewController:
    (nullable UIViewController*)viewController NS_UNAVAILABLE;
- (nullable instancetype)
initWithBaseViewController:(nullable UIViewController*)viewController
              browserState:(nullable ios::ChromeBrowserState*)browserState
    NS_UNAVAILABLE;

// Dispatcher.
@property(nonatomic, weak, nullable) id<ApplicationCommands, BrowserCommands>
    dispatcher;
// The web state list this ToolbarCoordinator is handling.
@property(nonatomic, assign, nonnull) WebStateList* webStateList;
// Delegate for this coordinator.
// TODO(crbug.com/799446): Change this.
@property(nonatomic, weak, nullable) id<ToolbarCoordinatorDelegate> delegate;
// URL loader for the toolbar.
// TODO(crbug.com/799446): Remove this.
@property(nonatomic, weak, nullable) id<UrlLoader> URLLoader;

@end

#endif  // IOS_CHROME_BROWSER_UI_TOOLBAR_ADAPTIVE_PRIMARY_TOOLBAR_COORDINATOR_H_
