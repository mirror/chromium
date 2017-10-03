// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <XCTest/XCTest.h>

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface WarmStartupTestCase : XCTestCase
@end

@implementation WarmStartupTestCase

- (void)testWarmSrartUp {
  XCUIApplication* app = [[XCUIApplication alloc] init];
  app.launchArguments = @[ @"testMode" ];
  [app launch];
  // Background the app.
  [[[XCUIDevice alloc] init] pressButton:XCUIDeviceButtonHome];
  // Bring the app to foreground.
  [app activate];
}
@end
