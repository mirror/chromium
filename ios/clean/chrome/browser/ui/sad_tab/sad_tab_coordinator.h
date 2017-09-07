// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_SAD_TAB_SAD_TAB_COORDINATOR_H_
#define IOS_CHROME_BROWSER_UI_SAD_TAB_SAD_TAB_COORDINATOR_H_

#import "ios/chrome/browser/ui/coordinators/browser_coordinator.h"

namespace web {
class WebState;
}

// Coordinator for displaying a SadTab.
@interface SadTabCoordinator : BrowserCoordinator
// The web state this SadTabCoordinator is handling.
@property(nonatomic, assign) web::WebState* webState;
// YES if it will display a SadTab for a repeated crash.
@property(nonatomic, assign) BOOL repeatedFailure;
@end

#endif  // IOS_CHROME_BROWSER_UI_SAD_TAB_SAD_TAB_COORDINATOR_H_
