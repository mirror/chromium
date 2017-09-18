// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>
#import <XCTest/XCTest.h>

#import "ios/chrome/test/earl_grey/chrome_test_case.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface TabGridTestCase : XCTestCase

@end

@implementation TabGridTestCase

- (void)testTabGrid {
  [[EarlGrey
      selectElementWithMatcher:grey_accessibilityID(@"incognitoToggleControl")]
      assertWithMatcher:grey_sufficientlyVisible()];
}

@end
