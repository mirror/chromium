// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_LEGACY_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_LEGACY_COORDINATOR_H_

#import "ios/chrome/browser/chrome_coordinator.h"

@class CommandDispatcher;
@protocol PageInfoPresentation;
@class TabModel;

namespace ios {
class ChromeBrowserState;
}  // namespace ios

@interface PageInfoLegacyCoordinator : ChromeCoordinator

@property(nonatomic, assign) ios::ChromeBrowserState* browserState;

@property(nonatomic, weak) CommandDispatcher* dispatcher;

@property(nonatomic, weak) id<PageInfoPresentation> presentationProvider;

@property(nonatomic, weak) TabModel* tabModel;

- (void)disconnect;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAGE_INFO_PAGE_INFO_LEGACY_COORDINATOR_H_
