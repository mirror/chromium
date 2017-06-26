// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/performance/startupLoggers.h"
#include "base/time/time.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Stores the time of app startup states.
base::Time* g_start_time;
base::Time* g_finish_launching_time;
base::Time* g_become_active_time;
}

@implementation startupLoggers

+ (instancetype)sharedInstance {
  static dispatch_once_t onceToken;
  static startupLoggers* sharedInstance;
  dispatch_once(&onceToken, ^{
    sharedInstance = [[self alloc] init];
  });
  return sharedInstance;
}

- (instancetype)init {
  self = [super init];
  return self;
}

- (void)registerAppStartTime {
  DCHECK(!g_start_time);
  g_start_time = new base::Time(base::Time::Now());
}

- (void)registerAppDidFinishLaunchingTime {
  DCHECK(g_start_time);
  DCHECK(!g_finish_launching_time);
  g_finish_launching_time = new base::Time(base::Time::Now());
}

- (void)registerAppDidBecomeActiveTime {
  DCHECK(g_start_time);
  DCHECK(!g_become_active_time);
  g_become_active_time = new base::Time(base::Time::Now());
}

- (BOOL)logData {
  NSMutableDictionary* dataDictionary = [[NSMutableDictionary alloc] init];
  double finishLaunchingDuration =
      g_finish_launching_time->ToDoubleT() - g_start_time->ToDoubleT();
  double becomeActiveDuration =
      g_become_active_time->ToDoubleT() - g_start_time->ToDoubleT();

  [dataDictionary setObject:[NSNumber numberWithDouble:finishLaunchingDuration]
                     forKey:@"AppDidFinishLaunchingTime"];
  [dataDictionary setObject:[NSNumber numberWithDouble:becomeActiveDuration]
                     forKey:@"AppDidBecomeActiveTime"];

  NSMutableDictionary* dataSummary = [[NSMutableDictionary alloc] init];
  [dataSummary setObject:dataDictionary forKey:@"Perf Data"];

  // Converts data into json format.
  NSError* error;
  NSData* jsonData =
      [NSJSONSerialization dataWithJSONObject:dataSummary
                                      options:NSJSONWritingPrettyPrinted
                                        error:&error];

  // Stores data into a json file under the app's Document directory.
  NSString* fileName = @"summary.json";
  NSString* outputPath = [NSSearchPathForDirectoriesInDomains(
      NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
  outputPath = [outputPath stringByAppendingPathComponent:fileName];

  // The summary.json may already exists if the app is not cleaned up.
  if ([[NSFileManager defaultManager] fileExistsAtPath:outputPath]) {
    [[NSFileManager defaultManager] removeItemAtPath:outputPath error:&error];
  }
  [jsonData writeToFile:outputPath atomically:YES];
  NSLog(@"Perf data file path: %@.", outputPath);

  return [[NSFileManager defaultManager] fileExistsAtPath:outputPath];
}

@end
