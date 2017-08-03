// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/find_in_page/find_in_page_metrics_recorder.h"

#include "base/test/user_action_tester.h"
#import "ios/clean/chrome/browser/ui/commands/find_in_page_visibility_commands.h"
#import "ios/shared/chrome/browser/ui/metrics/metrics_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Verifies that the FindInPageMetricsRecorder records the appropriate
// user action when |recordMetricForInvocation:| is called.
TEST(FindInPageMetricsRecorderTest, FindInPageUserActionRecorded) {
  base::UserActionTester user_action_tester;

  FindInPageMetricsRecorder* recorder =
      [[FindInPageMetricsRecorder alloc] init];
  NSInvocation* invocation =
      metrics_test_util::GetInvocationForRequiredProtocolInstanceMethod(
          @protocol(FindInPageVisibilityCommands), @selector(showFindInPage),
          YES);

  [recorder recordMetricForInvocation:invocation];

  EXPECT_EQ(1, user_action_tester.GetActionCount("MobileMenuFindInPage"));
}
