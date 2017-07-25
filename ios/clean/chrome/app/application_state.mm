// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/app/application_state.h"

#include <memory>

#import "ios/clean/chrome/app/steps/step_collections.h"
#import "ios/clean/chrome/app/steps/step_features.h"
#import "ios/clean/chrome/app/application_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif




@implementation ApplicationState
@dynamic phase;

@synthesize browserState = _browserState;
@synthesize window = _window;
@synthesize launchOptions = _launchOptions;

#pragma mark - Object lifecycle

- (void)configure {
  [self addSteps:[StepCollections allApplicationSteps]];
  [self requireFeature:step_features::kBrowserState
              forPhase:APPLICATION_BACKGROUNDED];
//  [self requireFeature:step_features::kBrowserStateInitialized
//              forPhase:APPLICATION_FOREGROUNDED];
  [self requireFeature:step_features::kMainWindow
              forPhase:APPLICATION_FOREGROUNDED];
    [self requireFeature:step_features::kChromeMainStopped
                forPhase:APPLICATION_TERMINATING];
}



@end
