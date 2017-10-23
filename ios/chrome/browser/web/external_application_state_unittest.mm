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
  EXPECT_EQ(ExternalAppLaunchPolicyAllow, [state launchPolicy]);

  // Simulate launching the app for the max allowed consecutive times.
  for (int i = 0; i < kMaxAllowedConsecutiveExternalAppLaunches; i++) {
    [state updateStateWithLaunchRequest];
    EXPECT_EQ(ExternalAppLaunchPolicyAllow, [state launchPolicy]);
  }
  // Once a try to launch the app again happens a prompt should appear.
  [state updateStateWithLaunchRequest];
  EXPECT_EQ(ExternalAppLaunchPolicyPrompt, [state launchPolicy]);
}

TEST_F(ExternalApplicationStateTest,
       TestAllowAfterMaxSecondsBetweenConsecutiveLaunches) {
  ExternalApplicationState* state = [[ExternalApplicationState alloc] init];
  // Update the max time between launches.
  double maxSecondsBetweenLaunches = 0.5;
  [ExternalApplicationState
      setMaxSecondsBetweenLaunches:maxSecondsBetweenLaunches];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow, [state launchPolicy]);

  // Simulate launching the app for the max allowed consecutive times.
  for (int i = 0; i < kMaxAllowedConsecutiveExternalAppLaunches; i++) {
    [state updateStateWithLaunchRequest];
    EXPECT_EQ(ExternalAppLaunchPolicyAllow, [state launchPolicy]);
  }
  // Wait for more than the max time between two consecutive runs
  [NSThread sleepForTimeInterval:maxSecondsBetweenLaunches + 0.1];
  // When the app tries to launch again it should be allowed normally.
  [state updateStateWithLaunchRequest];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow, [state launchPolicy]);
  [ExternalApplicationState setMaxSecondsBetweenLaunches:
                                kDefaultMaxSecondsBetweenConsecutiveLaunches];
}

TEST_F(ExternalApplicationStateTest, TestBlockApp) {
  ExternalApplicationState* state = [[ExternalApplicationState alloc] init];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow, [state launchPolicy]);
  [state updateStateWithLaunchRequest];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow, [state launchPolicy]);
  [state setStateBlocked];
  EXPECT_EQ(ExternalAppLaunchPolicyBlock, [state launchPolicy]);
}
