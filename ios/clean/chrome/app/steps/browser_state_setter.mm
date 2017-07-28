// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ======                        New Architecture                         =====
// =         This code is only used in the new iOS Chrome architecture.       =
// ============================================================================

#import "ios/clean/chrome/app/steps/browser_state_setter.h"

#include "base/logging.h"
#import "ios/clean/chrome/app/steps/step_context.h"
#import "ios/clean/chrome/app/steps/step_features.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state_manager.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BrowserStateSetter

- (instancetype)init {
  if ((self = [super init])) {
    self.providedFeature = step_features::kBrowserState;
    self.requiredFeatures = @[
      step_features::kChromeMainStarted, step_features::kBundleAndDefaults
    ];
  }
  return self;
}

- (void)runFeature:(NSString*)feature withContext:(id<StepContext>)context {
  NSLog(@"Run step 3 BrowserStateSetter");
  DCHECK([context respondsToSelector:@selector(setBrowserState:)]);
  context.browserState = GetApplicationContext()
                             ->GetChromeBrowserStateManager()
                             ->GetLastUsedBrowserState();
}

@end
