// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ======                        New Architecture                         =====
// =         This code is only used in the new iOS Chrome architecture.       =
// ============================================================================

#import "ios/clean/chrome/app/steps/browser_state_initializer.h"

#import "base/logging.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/content_settings/host_content_settings_map_factory.h"
#import "ios/clean/chrome/app/steps/step_context.h"
#import "ios/clean/chrome/app/steps/step_features.h"
#include "ios/net/cookies/cookie_store_ios.h"
#include "ios/web/public/web_capabilities.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@implementation BrowserStateInitializer

- (instancetype)init {
  if ((self = [super init])) {
    self.providedFeature = step_features::kBrowserStateInitialized;
    self.requiredFeatures = @[ step_features::kForeground ];
  }
  return self;
}

- (void)runFeature:(NSString*)feature withContext:(id<StepContext>)context {
  DCHECK(!context.browserState->IsOffTheRecord());
  [self setInitialCookiesPolicy:context.browserState];
}

// Copied verbatim from MainController.
- (void)setInitialCookiesPolicy:(ios::ChromeBrowserState*)browserState {
}

@end
