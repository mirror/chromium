// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Loggers for chrome startup performance testing.
@interface startupLoggers : NSObject

+ (instancetype)sharedInstance;

// Register the current time variable. Call the method when the app is launched.
- (void)registerAppStartTime;
// Register the time variable when the app gets didFinishLaunchingWithOptions
// notification.
- (void)registerAppDidFinishLaunchingTime;
// Chrome does some lauch option steps after the app gets
// didFinishLaunchingWithOptions notification. Register the time variable when
// the app enters applicationDidBecomeActive.
- (void)registerAppDidBecomeActiveTime;
// Log the stored time variables into a json file.
- (BOOL)logData;

@end
