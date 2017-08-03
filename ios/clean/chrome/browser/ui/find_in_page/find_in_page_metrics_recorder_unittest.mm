// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/clean/chrome/browser/ui/find_in_page/find_in_page_metrics_recorder.h"

#import <objc/runtime.h>

#include "base/test/user_action_tester.h"
#import "ios/clean/chrome/browser/ui/commands/find_in_page_visibility_commands.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
NSInvocation* InvocationForProtocol(Protocol* protocol,
                                    SEL selector,
                                    BOOL isRequiredMethod) {
  struct objc_method_description methodDesc = protocol_getMethodDescription(
      protocol, selector, isRequiredMethod, YES /* an instance method */);
  DCHECK(methodDesc.types);
  NSMethodSignature* method =
      [NSMethodSignature signatureWithObjCTypes:methodDesc.types];
  DCHECK(method);

  NSInvocation* invocation =
      [NSInvocation invocationWithMethodSignature:method];
  invocation.selector = selector;
  return invocation;
}
}

TEST(FindInPageMetricsRecorderTest, DoesItWork) {
  base::UserActionTester user_action_tester;

  FindInPageMetricsRecorder* recorder =
      [[FindInPageMetricsRecorder alloc] init];
  NSInvocation* invocation = InvocationForProtocol(
      @protocol(FindInPageVisibilityCommands), @selector(showFindInPage), YES);

  [recorder recordMetricForInvocation:invocation];

  EXPECT_EQ(1, user_action_tester.GetActionCount("MobileMenuFindInPage"));
}
