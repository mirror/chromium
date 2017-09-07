// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CLEAN_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_CONTAINER_COORDINATOR_H_
#define IOS_CLEAN_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_CONTAINER_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/browser_coordinator.h"
#import "ios/clean/chrome/browser/url_opening.h"

// Coordinator that contains a UI showing the two grids of tabs, one of each
// browser state (incognito/normal).
@interface TabGridContainerCoordinator : BrowserCoordinator<URLOpening>
@end

#endif  // IOS_CLEAN_CHROME_BROWSER_UI_TAB_GRID_TAB_GRID_CONTAINER_COORDINATOR_H_
