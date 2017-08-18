// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>
#import <XCTest/XCTest.h>

#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/testing/perf/startupLoggers.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test class for the Reading List menu.
@interface ChromePerfTestCase : ChromeTestCase

@end

@implementation ChromePerfTestCase

- (void)testChromeColdStartup {
  XCTAssertTrue(startup_loggers::LogData(@"testChromeColdStartup"));
}

@end
