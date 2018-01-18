// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/app_launcher/app_launcher_coordinator.h"

#import <UIKit/UIKit.h>

#include "ios/chrome/grit/ios_strings.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test fixture for AppLauncherCoordinator class.
class AppLauncherCoordinatorTest : public PlatformTest {
 protected:
  AppLauncherCoordinatorTest() {
    view_controller_ = OCMClassMock([UIViewController class]);
    coordinator_ = [[AppLauncherCoordinator alloc]
        initWithBaseViewController:view_controller_];
    application_ = OCMClassMock([UIApplication class]);
    OCMStub([application_ sharedApplication]).andReturn(application_);
  }

  id application_ = nil;
  id view_controller_ = nil;
  AppLauncherCoordinator* coordinator_ = nil;
};

// Tests that an empty URL does not prompt user and does not launch application.
TEST_F(AppLauncherCoordinatorTest, EmptyUrl) {
  OCMStub([[view_controller_ ignoringNonObjectArgs]
              presentViewController:[OCMArg any]
                           animated:YES
                         completion:[OCMArg any]])
      .andDo(^(NSInvocation* invocation) {
        ADD_FAILURE() << "Empty URL should not prompt user.";
      });

  BOOL app_exists = [coordinator_ appLauncherTabHelper:nullptr
                                      launchAppWithURL:GURL::EmptyGURL()
                                            linkTapped:NO];
  EXPECT_FALSE(app_exists);
}

// Tests that an invalid URL does not launch application.
TEST_F(AppLauncherCoordinatorTest, InvalidUrl) {
  BOOL app_exists = [coordinator_ appLauncherTabHelper:nullptr
                                      launchAppWithURL:GURL("invalid")
                                            linkTapped:NO];
  EXPECT_FALSE(app_exists);
}

// Tests that an itunes URL shows an alert.
TEST_F(AppLauncherCoordinatorTest, ItmsUrlShowsAlert) {
  OCMExpect([view_controller_
      presentViewController:[OCMArg checkWithBlock:^BOOL(id value) {
        EXPECT_TRUE([value isKindOfClass:[UIAlertController class]]);
        EXPECT_NSEQ(l10n_util::GetNSString(IDS_IOS_OPEN_IN_ANOTHER_APP),
                    [value message]);
        return YES;
      }]
                   animated:YES
                 completion:[OCMArg any]]);
  BOOL app_exists = [coordinator_ appLauncherTabHelper:nullptr
                                      launchAppWithURL:GURL("itms://1234")
                                            linkTapped:NO];
  EXPECT_TRUE(app_exists);
  [view_controller_ verify];
}

// Tests that an app URL attempts to launch the application.
TEST_F(AppLauncherCoordinatorTest, AppUrlLaunchesApp) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  OCMExpect([application_ openURL:[OCMArg isNotNil]]);
#pragma clang diagnostic pop
  [coordinator_ appLauncherTabHelper:nullptr
                    launchAppWithURL:GURL("some-app://1234")
                          linkTapped:NO];
  [application_ verify];
}

// Tests that |-appLauncherTabHelper:launchAppWithURL:linkTapped:| returns NO
// if there is no application that corresponds to a given URL.
TEST_F(AppLauncherCoordinatorTest, NoApplicationForUrl) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
  OCMStub([application_ openURL:[OCMArg any]]).andReturn(NO);
#pragma clang diagnostic pop
  BOOL app_exists =
      [coordinator_ appLauncherTabHelper:nullptr
                        launchAppWithURL:GURL("no-app-installed://1234")
                              linkTapped:NO];
  EXPECT_FALSE(app_exists);
}
