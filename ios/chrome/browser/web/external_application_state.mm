// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/external_application_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

const double kDefaultMaxSecondsBetweenConsecutiveLaunches = 30.0;
const int kMaxAllowedConsecutiveExternalAppLaunches = 2;

@interface ExternalApplicationState ()
@property(nonatomic, readonly) NSDate* lastAppLaunchTime;
@end

@implementation ExternalApplicationState {
  int _consecutiveLaunchesCount;
  BOOL _appLaunchingBlocked;
}

static double gMaxSecondsBetweenConsecutiveExternalAppLaunches =
    kDefaultMaxSecondsBetweenConsecutiveLaunches;

@synthesize lastAppLaunchTime = _lastAppLaunchTime;

+ (void)setMaxSecondsBetweenLaunches:(double)seconds {
  gMaxSecondsBetweenConsecutiveExternalAppLaunches = seconds;
}

- (void)setStateBlocked {
  _appLaunchingBlocked = YES;
}

- (void)updateStateWithLaunchRequest {
  if (_appLaunchingBlocked)
    return;
  NSTimeInterval interval =
      gMaxSecondsBetweenConsecutiveExternalAppLaunches + 1;
  if (_lastAppLaunchTime)
    interval = -_lastAppLaunchTime.timeIntervalSinceNow;
  if (interval > gMaxSecondsBetweenConsecutiveExternalAppLaunches)
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
