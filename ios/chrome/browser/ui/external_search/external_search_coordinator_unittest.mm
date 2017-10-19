// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/external_search/external_search_coordinator.h"

#import <UIKit/UIKit.h>

#import "ios/chrome/browser/ui/commands/external_search_commands.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using ExternalSearchCoordinatorTest = PlatformTest;

@interface ExternalSearchCoordinator (Testing)<ExternalSearchCommands>
@end

TEST_F(ExternalSearchCoordinatorTest, LaunchExternalSearch) {
  ExternalSearchCoordinator* coordinator =
      [[ExternalSearchCoordinator alloc] initWithBaseViewController:nil];
  id application = OCMClassMock([UIApplication class]);
  coordinator.application = application;
  NSURL* expectedURL = [NSURL URLWithString:@"testexternalsearch://"];
  if (@available(iOS 10, *)) {
    OCMExpect([application openURL:expectedURL
                           options:[OCMArg any]
                 completionHandler:[OCMArg any]]);
  } else {
    OCMExpect([application openURL:expectedURL]);
  }

  [coordinator launchExternalSearch];

  EXPECT_OCMOCK_VERIFY(application);
}
