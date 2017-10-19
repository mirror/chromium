// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/web/external_app_launching_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const double MAX_SECONDS_BETWEEN_CONSECUTIVE_LAUNCHES = 30.0;
const int MAX_ALLOWED_CONSECUTIVE_LAUNCHES = 2;
}  // namespace

@interface ExternalAppLaunchingState ()
@property(nonatomic, readonly) NSDate* lastAppLaunchTime;
@end

@implementation ExternalAppLaunchingState {
  int _consecutiveLaunchesCount;
  BOOL _appLaunchingBlocked;
}

@synthesize lastAppLaunchTime = _lastAppLaunchTime;

- (instancetype)init {
  self = [super init];
  if (self) {
    _consecutiveLaunchesCount = 0;
    _appLaunchingBlocked = NO;
  }
  return self;
}

- (void)block {
  _appLaunchingBlocked = YES;
}

- (void)update {
  if (_appLaunchingBlocked)
    return;
  NSTimeInterval interval = MAX_SECONDS_BETWEEN_CONSECUTIVE_LAUNCHES + 1;
  if (_lastAppLaunchTime)
    interval = -_lastAppLaunchTime.timeIntervalSinceNow;
  if (interval > MAX_SECONDS_BETWEEN_CONSECUTIVE_LAUNCHES)
    _consecutiveLaunchesCount = 1;
  else
    _consecutiveLaunchesCount++;
  _lastAppLaunchTime = [NSDate date];
}

- (LaunchAction)launchAction {
  if (_appLaunchingBlocked)
    return Block;

  if (_consecutiveLaunchesCount > MAX_ALLOWED_CONSECUTIVE_LAUNCHES)
    return Prompt;

  return Allow;
}

@end
