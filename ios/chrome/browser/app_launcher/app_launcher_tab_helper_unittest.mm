// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/app_launcher/app_launcher_tab_helper.h"

#import "ios/chrome/browser/app_launcher/app_launcher_tab_helper_delegate.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// An object that conforms to AppLauncherTabHelperDelegate for testing.
@interface TestAppLauncherTabHelperDelegate
    : NSObject<AppLauncherTabHelperDelegate>
// Number of times |-appLauncherTabHelper:launchAppWithURL:linkTapped| was
// called.
@property(nonatomic, assign) NSUInteger countOfInvocationsToLaunchApp;
@end

@implementation TestAppLauncherTabHelperDelegate
@synthesize countOfInvocationsToLaunchApp = _countOfInvocationsToLaunchApp;
- (BOOL)appLauncherTabHelper:(AppLauncherTabHelper*)tabHelper
            launchAppWithURL:(const GURL&)URL
                  linkTapped:(BOOL)linkTapped {
  self.countOfInvocationsToLaunchApp++;
  return YES;
}
- (void)appLauncherTabHelper:(AppLauncherTabHelper*)tabHelper
    showAlertOfRepeatedLaunchesWithCompletionHandler:
        (ProceduralBlockWithBool)completionHandler {
}
@end

// Test fixture for AppLauncherTabHelper class.
class AppLauncherTabHelperTest : public PlatformTest {
 protected:
  AppLauncherTabHelperTest()
      : delegate_([[TestAppLauncherTabHelperDelegate alloc] init]) {
    AppLauncherTabHelper::CreateForWebState(&web_state_);
    tab_helper_ = AppLauncherTabHelper::FromWebState(&web_state_);
    tab_helper_->SetDelegate(delegate_);
  }

  web::TestWebState web_state_;
  TestAppLauncherTabHelperDelegate* delegate_ = nil;
  AppLauncherTabHelper* tab_helper_;
};

// Tests that an empty URL does not invoke app launch.
TEST_F(AppLauncherTabHelperTest, EmptyUrl) {
  delegate_.countOfInvocationsToLaunchApp = 0U;
  tab_helper_->RequestToLaunchApp(GURL::EmptyGURL(), GURL::EmptyGURL(), false);
  EXPECT_EQ(0U, delegate_.countOfInvocationsToLaunchApp);
}

// Tests that an invalid URL does not invoke app launch.
TEST_F(AppLauncherTabHelperTest, InvalidUrl) {
  delegate_.countOfInvocationsToLaunchApp = 0U;
  tab_helper_->RequestToLaunchApp(GURL("invalid"), GURL::EmptyGURL(), false);
  EXPECT_EQ(0U, delegate_.countOfInvocationsToLaunchApp);
}

// Tests that a valid URL does invoke app launch.
TEST_F(AppLauncherTabHelperTest, ValidUrl) {
  delegate_.countOfInvocationsToLaunchApp = 0U;
  tab_helper_->RequestToLaunchApp(GURL("valid://1234"), GURL::EmptyGURL(),
                                  false);
  EXPECT_EQ(1U, delegate_.countOfInvocationsToLaunchApp);
}
