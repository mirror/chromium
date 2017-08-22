// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/captive_portal/captive_portal_login_coordinator.h"

#include "base/test/ios/wait_util.h"
#import "ios/chrome/browser/ssl/captive_portal_detector_tab_helper.h"
#import "ios/chrome/browser/web/chrome_web_test.h"
#include "url/gurl.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// Test fixture for BlockedPopupTabHelper class.
class CaptivePortalLoginCoordinatorTest : public ChromeWebTest {
 protected:
  void SetUp() override {
    ChromeWebTest::SetUp();
    CaptivePortalDetectorTabHelper::CreateForWebState(web_state());
  }
};

// Tests that invoking start and stop on the coordinator presents and dismisses
// the captive portal login request view controller, respectively.
TEST_F(CaptivePortalLoginCoordinatorTest, StartAndStop) {
  GURL landing_url("http://landingurl");
  UIWindow* key_window = [[UIApplication sharedApplication] keyWindow];
  UIViewController* root_view_controller = key_window.rootViewController;

  CaptivePortalLoginCoordinator* coordinator =
      [[CaptivePortalLoginCoordinator alloc]
          initWithBaseViewController:root_view_controller
                          landingURL:landing_url];

  void (^test_steps)(void) = ^{
    EXPECT_EQ(nil, root_view_controller.presentedViewController);

    [coordinator start];
    EXPECT_TRUE(root_view_controller.presentedViewController);

    [coordinator stop];
    base::test::ios::WaitUntilCondition(^{
      return !root_view_controller.presentedViewController;
    });
  };
  // Ensure any other presented controllers are dismissed before starting the
  // coordinator.
  [root_view_controller dismissViewControllerAnimated:NO completion:test_steps];
}
