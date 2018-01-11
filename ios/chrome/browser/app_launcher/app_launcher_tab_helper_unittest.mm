// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/app_launcher/app_launcher_tab_helper.h"

#include "base/memory/ptr_util.h"
#import "ios/chrome/browser/app_launcher/app_launcher_tab_helper_delegate.h"
#import "ios/chrome/browser/web/external_apps_launch_policy_decider.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// An object that conforms to AppLauncherTabHelperDelegate for testing.
@interface FakeAppLauncherTabHelperDelegate
    : NSObject<AppLauncherTabHelperDelegate>
// Number of times |-appLauncherTabHelper:launchAppWithURL:linkTapped| was
// called.
@property(nonatomic, assign) NSUInteger countOfInvocationsToLaunchApp;
// Number of times
// |-appLauncherTabHelper:showAlertOfRepeatedLaunchesWithCompletionHandler| was
// called.
@property(nonatomic, assign) NSUInteger countOfInvocationsToShowAlert;
// Simulates the user tapping the accept button when prompted via
// |-appLauncherTabHelper:showAlertOfRepeatedLaunchesWithCompletionHandler|.
@property(nonatomic, assign) BOOL simulateUserAcceptingPrompt;
@end

@implementation FakeAppLauncherTabHelperDelegate
@synthesize countOfInvocationsToLaunchApp = _countOfInvocationsToLaunchApp;
@synthesize countOfInvocationsToShowAlert = _countOfInvocationsToShowAlert;
@synthesize simulateUserAcceptingPrompt = _simulateUserAcceptingPrompt;
- (BOOL)appLauncherTabHelper:(AppLauncherTabHelper*)tabHelper
            launchAppWithURL:(const GURL&)URL
                  linkTapped:(BOOL)linkTapped {
  self.countOfInvocationsToLaunchApp++;
  return YES;
}
- (void)appLauncherTabHelper:(AppLauncherTabHelper*)tabHelper
    showAlertOfRepeatedLaunchesWithCompletionHandler:
        (ProceduralBlockWithBool)completionHandler {
  self.countOfInvocationsToShowAlert++;
  completionHandler(self.simulateUserAcceptingPrompt);
}
@end

// An ExternalAppsLaunchPolicyDecider for testing.
@interface FakeExternalAppsLaunchPolicyDecider : ExternalAppsLaunchPolicyDecider
@property(nonatomic, assign) ExternalAppLaunchPolicy policy;
@end

@implementation FakeExternalAppsLaunchPolicyDecider
@synthesize policy = _policy;
- (ExternalAppLaunchPolicy)launchPolicyForURL:(const GURL&)gURL
                            fromSourcePageURL:(const GURL&)sourcePageURL {
  return self.policy;
}
@end

// Subclass of AppLauncherTabHelper for testing.
class TestAppLauncherTabHelper : public AppLauncherTabHelper {
 public:
  TestAppLauncherTabHelper(web::WebState* web_state,
                           id<AppLauncherTabHelperDelegate> delegate,
                           ExternalAppsLaunchPolicyDecider* policy_decider)
      : AppLauncherTabHelper(web_state, delegate) {
    policy_decider_ = policy_decider;
  }
};

// Test fixture for AppLauncherTabHelper class.
class AppLauncherTabHelperTest : public PlatformTest {
 protected:
  AppLauncherTabHelperTest()
      : delegate_([[FakeAppLauncherTabHelperDelegate alloc] init]),
        policy_decider_([[FakeExternalAppsLaunchPolicyDecider alloc] init]) {
    tab_helper_ = base::MakeUnique<TestAppLauncherTabHelper>(
        &web_state_, delegate_, policy_decider_);
  }

  web::TestWebState web_state_;
  FakeAppLauncherTabHelperDelegate* delegate_ = nil;
  FakeExternalAppsLaunchPolicyDecider* policy_decider_ = nil;
  std::unique_ptr<TestAppLauncherTabHelper> tab_helper_;
};

// Tests that an empty URL does not invoke app launch.
TEST_F(AppLauncherTabHelperTest, EmptyUrl) {
  tab_helper_->RequestToLaunchApp(GURL::EmptyGURL(), GURL::EmptyGURL(), false);
  EXPECT_EQ(0U, delegate_.countOfInvocationsToLaunchApp);
}

// Tests that an invalid URL does not invoke app launch.
TEST_F(AppLauncherTabHelperTest, InvalidUrl) {
  tab_helper_->RequestToLaunchApp(GURL("invalid"), GURL::EmptyGURL(), false);
  EXPECT_EQ(0U, delegate_.countOfInvocationsToLaunchApp);
}

// Tests that a valid URL does invoke app launch.
TEST_F(AppLauncherTabHelperTest, ValidUrl) {
  policy_decider_.policy = ExternalAppLaunchPolicyAllow;
  tab_helper_->RequestToLaunchApp(GURL("valid://1234"), GURL::EmptyGURL(),
                                  false);
  EXPECT_EQ(1U, delegate_.countOfInvocationsToLaunchApp);
}

// Tests that a valid URL does not invoke app launch when launch policy is to
// block.
TEST_F(AppLauncherTabHelperTest, ValidUrlBlocked) {
  policy_decider_.policy = ExternalAppLaunchPolicyBlock;
  tab_helper_->RequestToLaunchApp(GURL("valid://1234"), GURL::EmptyGURL(),
                                  false);
  EXPECT_EQ(0U, delegate_.countOfInvocationsToLaunchApp);
}

// Tests that a valid URL shows an alert and invokes app launch when launch
// policy is to prompt and user accepts.
TEST_F(AppLauncherTabHelperTest, ValidUrlPromptUserAccepts) {
  policy_decider_.policy = ExternalAppLaunchPolicyPrompt;
  delegate_.simulateUserAcceptingPrompt = YES;
  tab_helper_->RequestToLaunchApp(GURL("valid://1234"), GURL::EmptyGURL(),
                                  false);
  EXPECT_EQ(1U, delegate_.countOfInvocationsToShowAlert);
  EXPECT_EQ(1U, delegate_.countOfInvocationsToLaunchApp);
}

// Tests that a valid URL does not invoke app launch when launch policy is to
// prompt and user rejects.
TEST_F(AppLauncherTabHelperTest, ValidUrlPromptUserRejects) {
  policy_decider_.policy = ExternalAppLaunchPolicyPrompt;
  delegate_.simulateUserAcceptingPrompt = NO;
  tab_helper_->RequestToLaunchApp(GURL("valid://1234"), GURL::EmptyGURL(),
                                  false);
  EXPECT_EQ(0U, delegate_.countOfInvocationsToLaunchApp);
}
