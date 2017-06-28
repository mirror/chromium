// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/testing/performance/startupLoggers.h"
#include "base/time/time.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace startupLoggers {
// Stores the time of app startup states.
base::Time* g_start_time;
base::Time* g_finish_launching_time;
base::Time* g_become_active_time;

void registerAppStartTime() {
  DCHECK(!g_start_time);
  g_start_time = new base::Time(base::Time::Now());
}

void registerAppDidFinishLaunchingTime() {
  DCHECK(g_start_time);
  DCHECK(!g_finish_launching_time);
  g_finish_launching_time = new base::Time(base::Time::Now());
}

void registerAppDidBecomeActiveTime() {
  DCHECK(g_start_time);
  DCHECK(!g_become_active_time);
  g_become_active_time = new base::Time(base::Time::Now());
}

BOOL logData(NSString* testName) {
  // Store the data into a format compatiable with infra scripts.
  NSMutableDictionary* dataDictionary = [[NSMutableDictionary alloc] init];
  [dataDictionary setObject:[[NSMutableDictionary alloc] initWithCapacity:2]
                     forKey:testName];
  [[dataDictionary objectForKey:testName] setObject:@"seconds" forKey:@"unit"];

  double finishLaunchingDuration =
      g_finish_launching_time->ToDoubleT() - g_start_time->ToDoubleT();
  double becomeActiveDuration =
      g_become_active_time->ToDoubleT() - g_start_time->ToDoubleT();
  NSMutableDictionary* valuesDictionary = [[NSMutableDictionary alloc] init];
  [valuesDictionary
      setObject:[NSNumber numberWithDouble:finishLaunchingDuration]
         forKey:@"AppDidFinishLaunchingTime"];
  [valuesDictionary setObject:[NSNumber numberWithDouble:becomeActiveDuration]
                       forKey:@"AppDidBecomeActiveTime"];

  [[dataDictionary objectForKey:testName] setObject:[valuesDictionary copy]
                                             forKey:@"value"];

  NSMutableDictionary* dataSummary = [[NSMutableDictionary alloc] init];
  [dataSummary setObject:[dataDictionary copy] forKey:@"Perf Data"];

  // Converts data into json format.
  NSError* error;
  NSData* jsonData =
      [NSJSONSerialization dataWithJSONObject:dataSummary
                                      options:NSJSONWritingPrettyPrinted
                                        error:&error];

  // Stores data into a json file under the app's Document directory.
  NSString* fileName = @"perf_result.json";
  NSString* outputPath = [NSSearchPathForDirectoriesInDomains(
      NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0];
  outputPath = [outputPath stringByAppendingPathComponent:fileName];

  // The summary.json may already exists if the app is not cleaned up.
  if ([[NSFileManager defaultManager] fileExistsAtPath:outputPath]) {
    [[NSFileManager defaultManager] removeItemAtPath:outputPath error:&error];
  }
  [jsonData writeToFile:outputPath atomically:YES];

  return [[NSFileManager defaultManager] fileExistsAtPath:outputPath];
}
}
