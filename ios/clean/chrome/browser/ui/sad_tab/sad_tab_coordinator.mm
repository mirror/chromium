// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/sad_tab/sad_tab_coordinator.h"

#import "ios/chrome/browser/ui/browser_list/browser.h"
#import "ios/chrome/browser/ui/coordinators/browser_coordinator+internal.h"
#import "ios/clean/chrome/browser/ui/sad_tab/sad_tab_controller.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface SadTabCoordinator ()
// The controller that will display the SadTab.
@property(nonatomic, strong) SadTabController* sadTabController;
@end

@implementation SadTabCoordinator
@synthesize repeatedFailure = _repeatedFailure;
@synthesize sadTabController = _sadTabController;
@synthesize webState = _webState;

- (void)start {
  if (self.started)
    return;
  self.sadTabController =
      [[SadTabController alloc] initWithWebState:self.webState];
  self.sadTabController.dispatcher =
      static_cast<id>(self.browser->dispatcher());
  [self.sadTabController presentSadTabForRepeatedFailure:self.repeatedFailure];
  [super start];
}

- (void)stop {
  [super stop];
}

@end
