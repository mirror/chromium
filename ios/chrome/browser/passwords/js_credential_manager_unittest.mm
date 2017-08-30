// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/js_credential_manager.h"

#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "ios/web/public/test/web_js_test.h"
#include "ios/web/public/test/web_test_with_web_state.h"
#include "url/origin.h"

namespace credential_manager {

namespace {

constexpr char kTestIconUrl[] = "https://www.google.com/favicon.ico";
constexpr char kTestWebOrigin[] = "https://example.com";

}  // namespace

// Tests for js_credential_manager.mm file. Its functions RejectPromiseWith* and
// ResolvePromiseWith* are tested as follows:
// 1. |credential_manager| early script is injected to the page.
// 2. A Promise is created. Depending on a test, it's |resolve| or |reject|
//   function is expected to be called. That function stores the result or error
//   in variable(s) with window.test_* prefix.
// 3. To check if JavaScript executed by JsCredentialManager was correct, we
//   check the values of window.test_* variable(s).
class JsCredentialManagerTest
    : public web::WebJsTest<web::WebTestWithWebState> {
 public:
  JsCredentialManagerTest()
      : web::WebJsTest<web::WebTestWithWebState>(@[ @"credential_manager" ]) {}
  void SetUp() override {
    WebTestWithWebState::SetUp();

    // Load empty HTML and inject |credential_manager| early script.
    LoadHtmlAndInject(@"<html></html>");
  }
  DISALLOW_COPY_AND_ASSIGN(JsCredentialManagerTest);
};

// Tests that ResolvePromiseWithCredentialInfo resolves the promise with
// JavaScript PasswordCredential object containing correct values.
TEST_F(JsCredentialManagerTest, ResolveWithPasswordCredential) {
  // Let requestId be 3.
  ExecuteJavaScript(
      @"__gCrWeb.credentialManager.createPromise_(3)."
       "then(function(result) {"
       "  window.test_credential_ = result;"
       "});");
  password_manager::CredentialInfo credential;
  credential.type = password_manager::CredentialType::CREDENTIAL_TYPE_PASSWORD;
  credential.id = base::ASCIIToUTF16("test@google.com");
  credential.name = base::ASCIIToUTF16("Test User");
  credential.icon = GURL(base::ASCIIToUTF16(kTestIconUrl));
  credential.password = base::ASCIIToUTF16("32njk \\4 s3cr3t \\n' 1");
  base::Optional<password_manager::CredentialInfo> optional_info =
      base::Optional<password_manager::CredentialInfo>(credential);
  ResolvePromiseWithCredentialInfo(web_state(), 3, optional_info);
  EXPECT_NSEQ(ExecuteJavaScript(@"typeof window.test_credential_"), @"object");
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_credential_.type"), @"password");
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_credential_.id"),
              @"test@google.com");
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_credential_.name"), @"Test User");
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_credential_.iconURL"),
              base::SysUTF16ToNSString(base::ASCIIToUTF16(kTestIconUrl)));
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_credential_.password"),
              @"32njk \\4 s3cr3t \\n' 1");
}

// Tests that ResolvePromiseWithCredentialInfo resolves the promise with
// JavaScript FederatedCredential object containing correct values.
TEST_F(JsCredentialManagerTest, ResolveWithFederatedCredential) {
  // Let requestId be 3.
  ExecuteJavaScript(
      @"__gCrWeb.credentialManager.createPromise_(3)."
       "then(function(result) {"
       "  window.test_credential_ = result;"
       "});");
  password_manager::CredentialInfo credential;
  credential.type = password_manager::CredentialType::CREDENTIAL_TYPE_FEDERATED;
  credential.id = base::ASCIIToUTF16("test@google.com");
  credential.name = base::ASCIIToUTF16("Test User");
  credential.icon = GURL(base::ASCIIToUTF16(kTestIconUrl));
  credential.federation = url::Origin(GURL(kTestWebOrigin));
  base::Optional<password_manager::CredentialInfo> optional_info =
      base::Optional<password_manager::CredentialInfo>(credential);
  ResolvePromiseWithCredentialInfo(web_state(), 3, optional_info);
  EXPECT_NSEQ(ExecuteJavaScript(@"typeof window.test_credential_"), @"object");
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_credential_.type"), @"federated");
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_credential_.id"),
              @"test@google.com");
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_credential_.name"), @"Test User");
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_credential_.iconURL"),
              base::SysUTF16ToNSString(base::ASCIIToUTF16(kTestIconUrl)));
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_credential_.provider"),
              base::SysUTF16ToNSString(base::ASCIIToUTF16(kTestWebOrigin)));
}

// Tests that ResolvePromiseWithCredentialInfo resolves the promise with void
// when optional CredentialInfo is null.
TEST_F(JsCredentialManagerTest, ResolveWithNullCredential) {
  // Let requestId be 3.
  ExecuteJavaScript(
      @"__gCrWeb.credentialManager.createPromise_(3)."
       "then(function(result) {"
       "  window.test_credential_ = result;"
       "});");
  base::Optional<password_manager::CredentialInfo> null_credential =
      base::Optional<password_manager::CredentialInfo>();
  ResolvePromiseWithCredentialInfo(web_state(), 3, null_credential);
  EXPECT_NSEQ(ExecuteJavaScript(@"typeof window.test_credential_"),
              @"undefined");
}

// Tests that ResolvePromiseWithVoid resolves the promise with no value.
TEST_F(JsCredentialManagerTest, ResolveWithVoid) {
  // Let requestId be equal 5.
  // Only when the promise is resolved with void, will the |window.test_result_|
  // be true.
  ExecuteJavaScript(
      @"__gCrWeb.credentialManager.createPromise_(5)."
       "then(function(result) {"
       "  window.test_result_ = (result == undefined);"
       "});");
  ResolvePromiseWithVoid(web_state(), 5);
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_result_"), @YES);
}

// Tests that RejectPromiseWithTypeError rejects the promise with TypeError and
// correct message.
TEST_F(JsCredentialManagerTest, RejectWithTypeError) {
  // Let requestId be equal 100.
  ExecuteJavaScript(
      @"__gCrWeb.credentialManager.createPromise_(100)."
       "catch(function(reason) {"
       "  window.test_result_valid_type_ = (reason instanceof TypeError);"
       "  window.test_result_message_ = reason.message;"
       "});");
  RejectPromiseWithTypeError(
      web_state(), 100, base::ASCIIToUTF16("message with \"quotation\" marks"));
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_result_valid_type_"), @YES);
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_result_message_"),
              @"message with \"quotation\" marks");
}

// Tests that RejectPromiseWithInvalidStateError rejects the promise with
// DOMException(message, INVALID_STATE_ERR), where |message| is correct message
// taken as argument.
TEST_F(JsCredentialManagerTest, RejectWithInvalidState) {
  // Let requestId be 0.
  ExecuteJavaScript(
      @"__gCrWeb.credentialManager.createPromise_(0)."
       "catch(function(reason) {"
       "  window.test_result_valid_type_ ="
       "    (reason.name == DOMException.INVALID_STATE_ERR);"
       "  window.test_result_message_ = reason.message;"
       "});");
  RejectPromiseWithInvalidStateError(
      web_state(), 0, base::ASCIIToUTF16("A 'get()' request is pending"));
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_result_valid_type_"), @YES);
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_result_message_"),
              @"A 'get()' request is pending");
}

// Tests that RejectPromiseWithNotSupportedError rejects the promise with
// DOMException(message, NOT_SUPPORTED_ERR), where |message| is correct message
// taken as argument.
TEST_F(JsCredentialManagerTest, RejectWithNotSupportedError) {
  // Let requestId be 0.
  ExecuteJavaScript(
      @"__gCrWeb.credentialManager.createPromise_(0)."
       "catch(function(reason) {"
       "  window.test_result_valid_type_ ="
       "    (reason.name == DOMException.NOT_SUPPORTED_ERR);"
       "  window.test_result_message_ = reason.message;"
       "});");
  RejectPromiseWithNotSupportedError(
      web_state(), 0,
      base::ASCIIToUTF16(
          "An error occured while talking to the credential manager."));
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_result_valid_type_"), @YES);
  EXPECT_NSEQ(ExecuteJavaScript(@"window.test_result_message_"),
              @"An error occured while talking to the credential manager.");
}

}  // namespace credential_manager
