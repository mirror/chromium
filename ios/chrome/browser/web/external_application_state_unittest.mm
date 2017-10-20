// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/web/external_application_state.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using ExternalApplicationStateTest = PlatformTest;

TEST_F(ExternalApplicationStateTest, TestPromptAfterMaxConsecutiveLaunches) {
  ExternalApplicationState* state = [[ExternalApplicationState alloc] init];
  EXPECT_EQ([state launchPolicy], ExternalAppLaunchPolicyAllow);

  // Simulate launching the app for the max allowed consecutive times.
  for (int i = 0; i < kMaxAllowedConsecutiveExternalAppLaunches; i++) {
    [state updateStateWithLaunchRequest];
    EXPECT_EQ([state launchPolicy], ExternalAppLaunchPolicyAllow);
  }
  // Once a try to launch the app again happens a prompt should appear.
  [state updateStateWithLaunchRequest];
  EXPECT_EQ([state launchPolicy], ExternalAppLaunchPolicyPrompt);
}

TEST_F(ExternalApplicationStateTest,
       TestAllowAfterMaxSecondsBetweenConsecutiveLaunches) {
  ExternalApplicationState* state = [[ExternalApplicationState alloc] init];
  EXPECT_EQ([state launchPolicy], ExternalAppLaunchPolicyAllow);

  // Simulate launching the app for the max allowed consecutive times.
  for (int i = 0; i < kMaxAllowedConsecutiveExternalAppLaunches; i++) {
    [state updateStateWithLaunchRequest];
    EXPECT_EQ([state launchPolicy], ExternalAppLaunchPolicyAllow);
  }
  // Wait for more than the max time between two consecutive runs
  [NSThread
      sleepForTimeInterval:kMaxSecondsBetweenConsecutiveExternalAppLaunches +
                           5];
  // When the app tries to launch again it should be allowed normally.
  [state updateStateWithLaunchRequest];
  EXPECT_EQ([state launchPolicy], ExternalAppLaunchPolicyAllow);
}

TEST_F(ExternalApplicationStateTest, TestBlockApp) {
  ExternalApplicationState* state = [[ExternalApplicationState alloc] init];
  EXPECT_EQ([state launchPolicy], ExternalAppLaunchPolicyAllow);
  [state updateStateWithLaunchRequest];
  EXPECT_EQ([state launchPolicy], ExternalAppLaunchPolicyAllow);
  [state setStateBlocked];
  EXPECT_EQ([state launchPolicy], ExternalAppLaunchPolicyBlock);
}
