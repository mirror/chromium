// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/external_application_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ExternalApplicationState ()
@property(nonatomic, readonly) NSDate* lastAppLaunchTime;
@end

@implementation ExternalApplicationState {
  int _consecutiveLaunchesCount;
  BOOL _appLaunchingBlocked;
}

@synthesize lastAppLaunchTime = _lastAppLaunchTime;

- (void)setStateBlocked {
  _appLaunchingBlocked = YES;
}

- (void)updateStateWithLaunchRequest {
  if (_appLaunchingBlocked)
    return;
  NSTimeInterval interval =
      kMaxSecondsBetweenConsecutiveExternalAppLaunches + 1;
  if (_lastAppLaunchTime)
    interval = -_lastAppLaunchTime.timeIntervalSinceNow;
  if (interval > kMaxSecondsBetweenConsecutiveExternalAppLaunches)
    _consecutiveLaunchesCount = 1;
  else
    _consecutiveLaunchesCount++;
  _lastAppLaunchTime = [NSDate date];
}

- (ExternalAppLaunchPolicy)launchPolicy {
  if (_appLaunchingBlocked)
    return ExternalAppLaunchPolicyBlock;

  if (_consecutiveLaunchesCount > kMaxAllowedConsecutiveExternalAppLaunches)
    return ExternalAppLaunchPolicyPrompt;

  return ExternalAppLaunchPolicyAllow;
}

@end
