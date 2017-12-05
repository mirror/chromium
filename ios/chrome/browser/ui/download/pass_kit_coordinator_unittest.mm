// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/download/pass_kit_coordinator.h"

#import <PassKit/PassKit.h>

#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/infobars/core/infobar.h"
#import "ios/chrome/browser/download/pass_kit_tab_helper.h"
#include "ios/chrome/browser/download/pass_kit_test_util.h"
#include "ios/chrome/browser/infobars/infobar_manager_impl.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/fakes/fake_pass_kit_tab_helper_delegate.h"
#import "ios/chrome/test/scoped_key_window.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/fakes/test_navigation_manager.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using testing::WaitUntilConditionOrTimeout;

// Test fixture for PassKitCoordinator class.
class PassKitCoordinatorTest : public PlatformTest {
 protected:
  PassKitCoordinatorTest()
      : base_view_controller_([[UIViewController alloc] init]),
        coordinator_([[PassKitCoordinator alloc]
            initWithBaseViewController:base_view_controller_]),
        delegate_([[FakePassKitTabHelperDelegate alloc]
            initWithWebState:&web_state_]),
        test_navigation_manager_(
            std::make_unique<web::TestNavigationManager>()) {
    PassKitTabHelper::CreateForWebState(&web_state_, delegate_);
    InfoBarManagerImpl::CreateForWebState(&web_state_);
    web_state_.SetNavigationManager(std::move(test_navigation_manager_));
    [scoped_key_window_.Get() setRootViewController:base_view_controller_];
  }

  UIViewController* base_view_controller_;
  PassKitCoordinator* coordinator_;
  web::TestWebState web_state_;
  FakePassKitTabHelperDelegate* delegate_;
  ScopedKeyWindow scoped_key_window_;
  std::unique_ptr<web::NavigationManager> test_navigation_manager_;
};

// Tests that PassKitCoordinator presents PKAddPassesViewController for the
// valid PKPass object.
TEST_F(PassKitCoordinatorTest, ValidPassKitObject) {
  std::string data = testing::GetTestPass();
  NSData* nsdata = [NSData dataWithBytes:data.c_str() length:data.size()];
  PKPass* pass = [[PKPass alloc] initWithData:nsdata error:nil];
  ASSERT_TRUE(pass);

  [coordinator_ passKitTabHelper:PassKitTabHelper::FromWebState(&web_state_)
            presentDialogForPass:pass
                        webState:&web_state_];

  EXPECT_TRUE(WaitUntilConditionOrTimeout(testing::kWaitForUIElementTimeout, ^{
    return [base_view_controller_.presentedViewController
        isKindOfClass:[PKAddPassesViewController class]];
  }));
}

// Tests that PassKitCoordinator presents error infobar for invalid PKPass
// object.
TEST_F(PassKitCoordinatorTest, InvalidPassKitObject) {
  [coordinator_ passKitTabHelper:PassKitTabHelper::FromWebState(&web_state_)
            presentDialogForPass:nil
                        webState:&web_state_];

  infobars::InfoBarManager* infobar_manager =
      InfoBarManagerImpl::FromWebState(&web_state_);
  ASSERT_EQ(1U, infobar_manager->infobar_count());
  infobars::InfoBar* infobar = infobar_manager->infobar_at(0);
  ASSERT_TRUE(infobar->delegate());
  auto* delegate = infobar->delegate()->AsConfirmInfoBarDelegate();
  ASSERT_TRUE(delegate);
  DCHECK_EQ(l10n_util::GetStringUTF16(IDS_IOS_GENERIC_PASSKIT_ERROR),
            delegate->GetMessageText());
}
