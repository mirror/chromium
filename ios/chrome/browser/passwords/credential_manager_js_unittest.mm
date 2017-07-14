// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/mac/foundation_util.h"
#include "ios/chrome/browser/passwords/js_credential_manager.h"
#import "ios/web/public/test/web_test_with_web_state.h"
#import "ios/web/public/web_state/js/crw_js_injection_receiver.h"
#import "ios/web/public/web_state/web_state.h"
#include "ios/web/public/web_state/web_state_observer.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

// TODO(crbug.com/435048) tgarbus: Write tests for handling messages received
// from JavaScript.

namespace {

using ::testing::_;

class CredentialManagerJsTest : public web::WebTestWithWebState {
 public:
  CredentialManagerJsTest(){};

  void SetUp() override {
    web::WebTestWithWebState::SetUp();
    js_credential_manager_ = base::mac::ObjCCastStrict<JSCredentialManager>(
        [web_state()->GetJSInjectionReceiver()
            instanceOfClass:[JSCredentialManager class]]);
  }

  // Sets up a web page and injects the JSCredentialManager.
  void Inject() {
    LoadHtml(@"<html></html>");
    [js_credential_manager_ inject];
  }

  JSCredentialManager* js_credential_manager() {
    return js_credential_manager_;
  }

 private:
  JSCredentialManager* js_credential_manager_;
};

// Tests that JSCredentialManager injects a Javascript which installs
// CredentialsContainer at window.navigator.credentials
TEST_F(CredentialManagerJsTest, InstallPublicCredentialsContainerInterface) {
  Inject();
  EXPECT_TRUE([js_credential_manager() hasBeenInjected]);

  // Check if the public interface has been installed
  EXPECT_NSEQ(@YES, ExecuteJavaScript(@"!!window.navigator.credentials"));

  // Check if the public interface contains the specified methods
  EXPECT_NSEQ(@YES, ExecuteJavaScript(@"!!window.navigator.credentials.get"));
  EXPECT_NSEQ(@YES, ExecuteJavaScript(@"!!window.navigator.credentials.store"));
  EXPECT_NSEQ(@YES, ExecuteJavaScript(
                        @"!!window.navigator.credentials.preventSilentAccess"));
  EXPECT_NSEQ(@YES,
              ExecuteJavaScript(@"!!window.navigator.credentials.create"));
}

}  // namespace
