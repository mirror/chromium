// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/web_view/internal/autofill/cwv_autofill_controller_internal.h"

#include <memory>

#include "base/memory/ptr_util.h"
#import "ios/web/public/test/fakes/crw_test_js_injection_receiver.h"
#import "ios/web/public/test/fakes/test_web_state.h"
#include "ios/web/public/test/test_web_thread_bundle.h"
#include "ios/web_view/internal/web_view_browser_state.h"
#include "testing/gtest/include/gtest/gtest.h"
#import "testing/gtest_mac.h"
#include "testing/platform_test.h"
#include "ui/base/l10n/l10n_util_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace ios_web_view {

class CWVAutofillControllerTest : public PlatformTest {
 protected:
  void SetUp() override {
    PlatformTest::SetUp();

    l10n_util::OverrideLocaleWithCocoaLocale();

    webState_.reset(new web::TestWebState());

    browserState_.reset(new ios_web_view::WebViewBrowserState(false));
    webState_->SetBrowserState(browserState_.get());

    CRWTestJSInjectionReceiver* injectionReceiver =
        [[CRWTestJSInjectionReceiver alloc] init];
    webState_->SetJSInjectionReceiver(injectionReceiver);
  }

  void TearDown() override {
    PlatformTest::TearDown();

    browserState_.reset();
    webState_.reset();
  }

  std::unique_ptr<web::TestWebState> webState_;
  std::unique_ptr<ios_web_view::WebViewBrowserState> browserState_;

 private:
  web::TestWebThreadBundle web_thread_bundle_;
};

// Tests CWVAutofillController dependency injection.
TEST_F(CWVAutofillControllerTest, DependencyInjection) {
  CWVAutofillController* autofillController =
      [[CWVAutofillController alloc] init];
  EXPECT_FALSE(autofillController.webState);
  EXPECT_FALSE(autofillController.autofillClient);
  EXPECT_FALSE(autofillController.autofillAgent);
  EXPECT_FALSE(autofillController.autofillManager);
  EXPECT_FALSE(autofillController.JSAutofillManager);

  autofillController.webState = webState_.get();
  EXPECT_TRUE(autofillController.webState);
  EXPECT_TRUE(autofillController.autofillClient);
  EXPECT_TRUE(autofillController.autofillAgent);
  EXPECT_TRUE(autofillController.autofillManager);
  EXPECT_TRUE(autofillController.JSAutofillManager);
}

}  // namespace ios_web_view
