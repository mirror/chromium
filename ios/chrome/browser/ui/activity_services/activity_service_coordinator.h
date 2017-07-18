// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_ACTIVITY_SERVICE_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_ACTIVITY_SERVICE_COORDINATOR_H_

#import "ios/chrome/browser/chrome_coordinator.h"

@protocol ActivityServicePasswords;
@protocol ActivityServicePositioner;
@protocol ActivityServicePresentation;
@protocol ActivityServiceSnackbar;
@class CommandDispatcher;
@class TabModel;

namespace ios {
class ChromeBrowserState;
}  // namespace

@interface ActivityServiceCoordinator : ChromeCoordinator

@property(nonatomic, readwrite, assign) ios::ChromeBrowserState* browserState;
@property(nonatomic, readwrite, weak) CommandDispatcher* dispatcher;
@property(nonatomic, readwrite, weak) TabModel* tabModel;

@property(nonatomic, readwrite, weak) id<ActivityServicePositioner>
    positionProvider;
@property(nonatomic, readwrite, weak) id<ActivityServicePresentation>
    presentationProvider;
@property(nonatomic, readwrite, weak) id<ActivityServiceSnackbar>
    snackbarProvider;

- (void)disconnect;

- (void)cancelShare;

@end

#endif  // IOS_CHROME_BROWSER_UI_ACTIVITY_SERVICES_ACTIVITY_SERVICE_COORDINATOR_H_
