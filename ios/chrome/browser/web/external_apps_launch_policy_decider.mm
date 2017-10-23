// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/external_apps_launch_policy_decider.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/web/external_app_launching_state.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

const int kMaxAllowedConsecutiveExternalAppLaunches = 2;

@interface ExternalAppsLaunchPolicyDecider ()
// Maps between external application redirection key and state.
// the key is a space separated combination of the absloute string for the
// original source URL, and the scheme of the external Application URL.
@property(nonatomic, strong)
    NSMutableDictionary<NSString*, ExternalAppLaunchingState*>*
        appLaunchingStates;
// Generate key for |AppGURL| and |sourceURL| to be used to reterieve state from
// |appLaunchingStates|.
+ (NSString*)getStateKey:(const GURL&)AppGURL soureURL:(const GURL&)sourceURL;
@end

@implementation ExternalAppsLaunchPolicyDecider

@synthesize appLaunchingStates = _appLaunchingStates;

+ (NSString*)getStateKey:(const GURL&)AppGURL
                soureURL:(const GURL&)sourcePageURL {
  return base::SysUTF8ToNSString(sourcePageURL.GetContent() + " " +
                                 AppGURL.scheme());
}

- (instancetype)init {
  self = [super init];
  if (self) {
    _appLaunchingStates = [[NSMutableDictionary alloc] init];
  }
  return self;
}

- (void)didRequestLaunchExternalAppURL:(const GURL&)gURL
                     fromSourcePageURL:(const GURL&)sourcePageURL {
  NSString* key =
      [ExternalAppsLaunchPolicyDecider getStateKey:gURL soureURL:sourcePageURL];
  if (_appLaunchingStates[key] == nil)
    _appLaunchingStates[key] = [[ExternalAppLaunchingState alloc] init];
  [_appLaunchingStates[key] updateWithLaunchRequest];
}

- (ExternalAppLaunchPolicy)launchPolicyForURL:(const GURL&)gURL
                            fromSourcePageURL:(const GURL&)sourcePageURL {
  NSString* key =
      [ExternalAppsLaunchPolicyDecider getStateKey:gURL soureURL:sourcePageURL];
  // Don't block apps that are not registered with the policy decider.
  if (_appLaunchingStates[key] == nil)
    return ExternalAppLaunchPolicyAllow;

  ExternalAppLaunchingState* state = _appLaunchingStates[key];
  if ([state isAppLaunchingBlocked])
    return ExternalAppLaunchPolicyBlock;

  if (state.consecutiveLaunchesCount >
      kMaxAllowedConsecutiveExternalAppLaunches)
    return ExternalAppLaunchPolicyPrompt;

  return ExternalAppLaunchPolicyAllow;
}

- (void)blockLaunchingAppURL:(const GURL&)gURL
           fromSourcePageURL:(const GURL&)sourcePageURL {
  NSString* key =
      [ExternalAppsLaunchPolicyDecider getStateKey:gURL soureURL:sourcePageURL];
  // Can't update state for non existing key.
  if (_appLaunchingStates[key] == nil)
    return;
  [_appLaunchingStates[key] setAppLaunchingBlocked:YES];
}
@end
