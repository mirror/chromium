// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/web/external_apps_launch_policy_decider.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

const GURL kSourceURL1("http://www.google.com");
const GURL kSourceURL2("http://www.goog.com");
const GURL kSourceURL3("http://www.goog.ab");
const GURL kSourceURL4("http://www.foo.com");
const GURL kAppURL1("facetime://+1354");
const GURL kAppURL2("facetime-audio://+1234");
const GURL kAppURL3("abc://abc");
const GURL kAppURL4("chrome://www.google.com");

using ExternalAppsLaunchPolicyDeciderTest = PlatformTest;

// Test cases when the same app is launched repeatedly from same source.
TEST_F(ExternalAppsLaunchPolicyDeciderTest,
       TestRepeatedAppLaunches_SameAppSameSource) {
  ExternalAppsLaunchPolicyDecider* policyDecider =
      [[ExternalAppsLaunchPolicyDecider alloc] init];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:GURL("facetime://+154")
                            fromSourcePageURL:kSourceURL1]);

  [policyDecider didRequestLaunchExternalAppURL:GURL("facetime://+1354")
                              fromSourcePageURL:kSourceURL1];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:GURL("facetime://+12354")
                            fromSourcePageURL:kSourceURL1]);

  [policyDecider didRequestLaunchExternalAppURL:GURL("facetime://+154")
                              fromSourcePageURL:kSourceURL1];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:GURL("facetime://+13454")
                            fromSourcePageURL:kSourceURL1]);

  [policyDecider didRequestLaunchExternalAppURL:GURL("facetime://+14")
                              fromSourcePageURL:kSourceURL1];
  // App was launched more than the max allowed times, the policy should change
  // to Prompt.
  EXPECT_EQ(ExternalAppLaunchPolicyPrompt,
            [policyDecider launchPolicyForURL:GURL("facetime://+14")
                            fromSourcePageURL:kSourceURL1]);
}

// Test cases when same app is launched repeatedly from different sources.
TEST_F(ExternalAppsLaunchPolicyDeciderTest,
       TestRepeatedAppLaunches_SameAppDifferentSources) {
  ExternalAppsLaunchPolicyDecider* policyDecider =
      [[ExternalAppsLaunchPolicyDecider alloc] init];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL1
                            fromSourcePageURL:kSourceURL1]);
  [policyDecider didRequestLaunchExternalAppURL:kAppURL1
                              fromSourcePageURL:kSourceURL1];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL1
                            fromSourcePageURL:kSourceURL1]);

  [policyDecider didRequestLaunchExternalAppURL:kAppURL1
                              fromSourcePageURL:kSourceURL2];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL1
                            fromSourcePageURL:kSourceURL2]);
  [policyDecider didRequestLaunchExternalAppURL:kAppURL1
                              fromSourcePageURL:kSourceURL3];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL1
                            fromSourcePageURL:kSourceURL3]);
  [policyDecider didRequestLaunchExternalAppURL:kAppURL1
                              fromSourcePageURL:kSourceURL4];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL1
                            fromSourcePageURL:kSourceURL4]);
}

// Test cases when different apps are launched from different sources.
TEST_F(ExternalAppsLaunchPolicyDeciderTest,
       TestRepeatedAppLaunches_DifferentAppsDifferentSources) {
  ExternalAppsLaunchPolicyDecider* policyDecider =
      [[ExternalAppsLaunchPolicyDecider alloc] init];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL1
                            fromSourcePageURL:kSourceURL1]);
  [policyDecider didRequestLaunchExternalAppURL:kAppURL1
                              fromSourcePageURL:kSourceURL1];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL1
                            fromSourcePageURL:kSourceURL1]);

  [policyDecider didRequestLaunchExternalAppURL:kAppURL2
                              fromSourcePageURL:kSourceURL2];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL2
                            fromSourcePageURL:kSourceURL2]);
  [policyDecider didRequestLaunchExternalAppURL:kAppURL3
                              fromSourcePageURL:kSourceURL3];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL3
                            fromSourcePageURL:kSourceURL3]);
  [policyDecider didRequestLaunchExternalAppURL:kAppURL4
                              fromSourcePageURL:kSourceURL4];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL4
                            fromSourcePageURL:kSourceURL4]);
}

// Test blocking App launch only when the app have been launched through the
// policy decider before.
TEST_F(ExternalAppsLaunchPolicyDeciderTest, TestBlockLaunchingApp) {
  ExternalAppsLaunchPolicyDecider* policyDecider =
      [[ExternalAppsLaunchPolicyDecider alloc] init];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL1
                            fromSourcePageURL:kSourceURL1]);
  // Don't block for apps that have not been registered.
  [policyDecider blockLaunchingAppURL:kAppURL1 fromSourcePageURL:kSourceURL1];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL1
                            fromSourcePageURL:kSourceURL1]);

  // Block for apps that have been registered
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL2
                            fromSourcePageURL:kSourceURL2]);
  [policyDecider didRequestLaunchExternalAppURL:kAppURL2
                              fromSourcePageURL:kSourceURL2];
  EXPECT_EQ(ExternalAppLaunchPolicyAllow,
            [policyDecider launchPolicyForURL:kAppURL2
                            fromSourcePageURL:kSourceURL2]);
  [policyDecider blockLaunchingAppURL:kAppURL2 fromSourcePageURL:kSourceURL2];
  EXPECT_EQ(ExternalAppLaunchPolicyBlock,
            [policyDecider launchPolicyForURL:kAppURL2
                            fromSourcePageURL:kSourceURL2]);
}

}  // namespace
