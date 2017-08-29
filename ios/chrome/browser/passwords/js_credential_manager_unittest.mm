// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "ios/chrome/browser/passwords/js_credential_manager.h"

#include "base/strings/utf_string_conversions.h"
#include "ios/web/public/test/web_js_test.h"
#include "ios/web/public/test/web_test_with_web_state.h"

namespace credential_manager {

namespace {}  // namespace

class JsCredentialManagerTest
    : public web::WebJsTest<web::WebTestWithWebState> {
 public:
  JsCredentialManagerTest()
      : web::WebJsTest<web::WebTestWithWebState>(@[ @"credential_manager" ]) {}
  void SetUp() override {
    WebTestWithWebState::SetUp();

    manager_ = new JsCredentialManager(web_state());

    LoadHtmlAndInject(@"<html></html>");
  }

 protected:
  JsCredentialManager* manager_;
};

// Tests that ResolvePromiseWithVoid resolves the promise with no value.
TEST_F(JsCredentialManagerTest, ResolveWithVoid) {
  // Let requestId be equal 5.
  // Only when the promise is resolved with void, will the |window.test_result_|
  // be true.
  ExecuteJavaScript(
      @"__gCrWeb.credentialManager.createPromise_(5)."
       "then(function(result) {"
       "window.test_result_ = (result == undefined);"
       "});");
  manager_->ResolvePromiseWithVoid(5);
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_result_"), @YES);
}

// Tests that RejectPromiseWithTypeError resolves the promise with TypeError and
// correct message.
TEST_F(JsCredentialManagerTest, RejectWithTypeError) {
  // Let requestId be equal 100.
  ExecuteJavaScript(
      @"__gCrWeb.credentialManager.createPromise_(100)."
       "catch(function(reason) {"
       "window.test_result_valid_type_ = (reason instanceof TypeError);"
       "window.test_result_message_ = reason.message;"
       "});");
  manager_->RejectPromiseWithTypeError(
      100, base::ASCIIToUTF16("message with \"quotation\" marks"));
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_result_valid_type_"), @YES);
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_result_message_"),
              @"message with \"quotation\" marks");
}

}  // namespace credential_manager
