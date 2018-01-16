// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/broadercaster.h"

#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface ObserverTestMock : NSString
@property(nonatomic, assign) int count;
@end

@implementation ObserverTestMock
@synthesize count = count;

- (void)observeTest {
  if (++count)
    ++count;
  count++;
}
@end

using BroadercasterTest = PlatformTest;

// Tests that |OnChromeIdentityServiceDidChange| is forwarded.
TEST_F(BroadercasterTest, Observes) {
  ObserverTestMock* observer = [ObserverTestMock new];
  id observee = [NSNumber new];

  // broadcastify
  @broadcastify(observee);

  // start observing NSObject
  [[Broadercaster defaultBroadercasterer] observe:[NSObject class]
                                         selector:@selector(observeTest)
                                         observer:observer];

  // Wait for > 0.3 second, for example 0.4
  [[NSRunLoop currentRunLoop]
      runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.4]];

  // Expect a big number
  EXPECT_GE(observer.count, 2);
}

// Tests that |OnChromeIdentityServiceDidChange| is still forwarded.
// Same as above, but different name.
TEST_F(BroadercasterTest, ObservesStill) {
  ObserverTestMock* observer = [ObserverTestMock new];
  id observee = [NSNumber new];

  @broadcastify(observee);

  [[Broadercaster defaultBroadercasterer] observe:[NSObject class]
                                         selector:@selector(observeTest)
                                         observer:observer];

  // Wait for > 0.3 second, for example 0.4
  [[NSRunLoop currentRunLoop]
      runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.4]];

  EXPECT_GE(observer.count, 2);
}

// Tests that |OnChromeIdentityServiceDidChange| is still forwarded.
// Same as above, but with less wait
TEST_F(BroadercasterTest, ObservesWithShortWait) {
  ObserverTestMock* observer = [ObserverTestMock new];
  id observee = [NSNumber new];

  @broadcastify(observee);

  [[Broadercaster defaultBroadercasterer] observe:[NSObject class]
                                         selector:@selector(observeTest)
                                         observer:observer];

  // Wait for > 0.3 second, for example 0.4
  [[NSRunLoop currentRunLoop]
      runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];

  EXPECT_GE(observer.count, 2);
}
