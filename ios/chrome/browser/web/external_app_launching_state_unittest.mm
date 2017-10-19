// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/web/external_app_launching_state.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using ExternalAppLaunchingStateTest = PlatformTest;

TEST(ExternalAppLaunchingStateTest, TestPromptAfterMaxConsecutiveLaunches) {
  ExternalAppLaunchingState* state = [[ExternalAppLaunchingState alloc] init];
  EXPECT_EQ([state launchAction], Allow);
  [state update];
  EXPECT_EQ([state launchAction], Allow);
  [state update];
  EXPECT_EQ([state launchAction], Allow);
  [state update];
  EXPECT_EQ([state launchAction], Prompt);
}

TEST(ExternalAppLaunchingStateTest,
     TestAllowAfterMaxSecondsBetweenConsecutiveLaunches) {
  ExternalAppLaunchingState* state = [[ExternalAppLaunchingState alloc] init];
  EXPECT_EQ([state launchAction], Allow);
  [state update];
  EXPECT_EQ([state launchAction], Allow);
  [state update];
  EXPECT_EQ([state launchAction], Allow);
  [NSThread sleepForTimeInterval:35];
  [state update];
  EXPECT_EQ([state launchAction], Allow);
  [state update];
  EXPECT_EQ([state launchAction], Allow);
}

TEST(ExternalAppLaunchingStateTest, TestBlockApp) {
  ExternalAppLaunchingState* state = [[ExternalAppLaunchingState alloc] init];
  EXPECT_EQ([state launchAction], Allow);
  [state update];
  EXPECT_EQ([state launchAction], Allow);
  [state block];
  EXPECT_EQ([state launchAction], Block);
}
