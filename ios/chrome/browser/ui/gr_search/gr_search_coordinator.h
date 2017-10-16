// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_GR_SEARCH_GR_SEARCH_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_GR_SEARCH_GR_SEARCH_COORDINATOR_H_

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/chrome_coordinator.h"

@class CommandDispatcher;

@interface GRSearchCoordinator : ChromeCoordinator

// The application object used to open the GR search app. Default is
// UIApplication.sharedApplication.
@property(nonatomic, null_resettable) UIApplication* application;

// The dispatcher for this coordinator. When |dispatcher| is set, the
// coordinator will register itself as the target for PageInfoCommands.
@property(nonatomic, nullable, weak) CommandDispatcher* dispatcher;

// Stops listening for dispatcher calls and clears this coordinator's dispatcher
// property.
- (void)disconnect;

@end

#endif  // IOS_CHROME_BROWSER_UI_GR_SEARCH_GR_SEARCH_COORDINATOR_H_
