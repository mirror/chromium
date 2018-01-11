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

// Coordinator subclass for testing.
@interface FakeAppLauncherCoordinator : AppLauncherCoordinator
// The number of times
// |-showAlertWithMessage:acceptActionTitle:rejectActionTitle:completionHandler:|
// was called.
@property(nonatomic, assign) NSUInteger countOfInvocationsToShowAlert;
// Simulates the user tapping the accept button when prompted via
// |-showAlertWithMessage:acceptActionTitle:rejectActionTitle:completionHandler:|.
@property(nonatomic, assign) BOOL simulateUserAcceptingPrompt;
// Records the |message| from the last call to
// |-showAlertWithMessage:acceptActionTitle:rejectActionTitle:completionHandler:|.
@property(nonatomic, copy) NSString* lastShownAlertMessage;
- (void)showAlertWithMessage:(NSString*)message
           acceptActionTitle:(NSString*)acceptActionTitle
           rejectActionTitle:(NSString*)rejectActionTitle
           completionHandler:(ProceduralBlockWithBool)completionHandler;
@end

@implementation FakeAppLauncherCoordinator
@synthesize countOfInvocationsToShowAlert = _countOfInvocationsToShowAlert;
@synthesize simulateUserAcceptingPrompt = _simulateUserAcceptingPrompt;
@synthesize lastShownAlertMessage = _lastShownAlertMessage;
- (void)showAlertWithMessage:(NSString*)message
           acceptActionTitle:(NSString*)acceptActionTitle
           rejectActionTitle:(NSString*)rejectActionTitle
           completionHandler:(ProceduralBlockWithBool)completionHandler {
  self.countOfInvocationsToShowAlert++;
  self.lastShownAlertMessage = message;
  completionHandler(self.simulateUserAcceptingPrompt);
}
@end

// Test fixture for AppLauncherCoordinator class.
class AppLauncherCoordinatorTest : public PlatformTest {
 protected:
  AppLauncherCoordinatorTest() {
    coordinator_ =
        [[FakeAppLauncherCoordinator alloc] initWithBaseViewController:nil];
    application_ = OCMClassMock([UIApplication class]);
    OCMStub([application_ sharedApplication]).andReturn(application_);
  }

  id application_ = nil;
  FakeAppLauncherCoordinator* coordinator_ = nil;
};

// Tests that an empty URL does not launch application.
TEST_F(AppLauncherCoordinatorTest, EmptyUrl) {
  BOOL app_exists = [coordinator_ appLauncherTabHelper:nullptr
                                      launchAppWithURL:GURL::EmptyGURL()
                                            linkTapped:NO];
  EXPECT_FALSE(app_exists);
  EXPECT_EQ(0U, coordinator_.countOfInvocationsToShowAlert);
}

// Tests that an invalid URL does not launch application.
TEST_F(AppLauncherCoordinatorTest, InvalidUrl) {
  BOOL app_exists = [coordinator_ appLauncherTabHelper:nullptr
                                      launchAppWithURL:GURL("invalid")
                                            linkTapped:NO];
  EXPECT_FALSE(app_exists);
}

// Tests that an itunes URL shows an alert and launches application when user
// accepts prompt.
TEST_F(AppLauncherCoordinatorTest, UserAcceptsLaunchingItmsUrl) {
  coordinator_.simulateUserAcceptingPrompt = YES;
  OCMExpect([application_ openURL:[OCMArg isNotNil]
                          options:@{}
                completionHandler:nil]);
  BOOL app_exists = [coordinator_ appLauncherTabHelper:nullptr
                                      launchAppWithURL:GURL("itms://1234")
                                            linkTapped:NO];
  EXPECT_TRUE(app_exists);
  EXPECT_EQ(1U, coordinator_.countOfInvocationsToShowAlert);
  NSString* expectedMessage =
      l10n_util::GetNSString(IDS_IOS_OPEN_IN_ANOTHER_APP);
  EXPECT_NSEQ(expectedMessage, coordinator_.lastShownAlertMessage);
  [application_ verify];
}

// Tests that iTunes is not launched when user rejects prompt.
TEST_F(AppLauncherCoordinatorTest, UserRejectsLaunchingItmsUrl) {
  coordinator_.simulateUserAcceptingPrompt = NO;
  OCMStub([application_ openURL:[OCMArg any]
                        options:[OCMArg any]
              completionHandler:[OCMArg any]])
      .andDo(^(NSInvocation* invocation) {
        ADD_FAILURE() << "Application should not be launched.";
      });
  [coordinator_ appLauncherTabHelper:nullptr
                    launchAppWithURL:GURL("itms://1234")
                          linkTapped:NO];
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

// Tests that |-appLauncherTabHelper:launchAppWithURL:linkTapped:| returns NO if
// there is no application that corresponds to a given URL.
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
