// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web/navigation/crw_placeholder_navigation_info.h"

#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "third_party/ocmock/OCMock/OCMock.h"

// Test fixture for CRWPlaceholderNavigationInfo.
typedef PlatformTest CRWPlaceholderNavigationInfoTest;

// Tests that completion handler is saved and retrieved from a navigation.
TEST_F(CRWPlaceholderNavigationInfoTest, SetInfoForNavigation) {
  WKNavigation* navigation = OCMClassMock([WKNavigation class]);
  EXPECT_NSEQ(nil,
              [CRWPlaceholderNavigationInfo getInfoForNavigation:navigation]);

  __block BOOL called = NO;
  ProceduralBlock completionHandler = ^{
    called = YES;
  };

  [CRWPlaceholderNavigationInfo setInfoForNavigation:navigation
                                   completionHandler:completionHandler];
  EXPECT_EQ(NO, called);

  CRWPlaceholderNavigationInfo* info =
      [CRWPlaceholderNavigationInfo getInfoForNavigation:navigation];
  ASSERT_NSNE(nil, info);
  EXPECT_EQ(NO, called);

  [info runCompletionHandler];
  EXPECT_EQ(YES, called);
}

TEST_F(CRWPlaceholderNavigationInfoTest, GetInfoOnNilNavigationReturnsNil) {
  EXPECT_NSEQ(nil, [CRWPlaceholderNavigationInfo getInfoForNavigation:nil]);
}
