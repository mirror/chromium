// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/credential_manager.h"

#import <EarlGrey/EarlGrey.h>
#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/earl_grey/accessibility_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#include "base/test/scoped_feature_list.h"
#include "ios/chrome/browser/passwords/credential_manager_features.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#import "ios/web/public/test/http_server/html_response_provider.h"
#import "ios/web/public/test/http_server/html_response_provider_impl.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"
#include "ios/chrome/browser/passwords/ios_chrome_password_store_factory.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "ios/chrome/grit/ios_strings.h"
#include "base/test/ios/wait_util.h"
#import "ios/testing/wait_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Time that is allowed to elapse between calling API method from JS and UI being displayed.
const CFTimeInterval kSnackbarAppearanceTimeout = 5;
// Snackbar should be displayed for 3 seconds. 3.5 is safer timeout to check and also smaller than 4, which is default value for MDCSnackbarMessage.duration.
const CFTimeInterval kSnackbarDisappearanceTimeout = 3.5;

}  // namespace

@interface CredentialManagerAutoSigninTestCase : ChromeTestCase

@end

@implementation CredentialManagerAutoSigninTestCase

- (void)setUp {
  [super setUp];
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(features::kCredentialManager);
  DLOG(ERROR) << "Set Up";

  // Loads simple page.
  std::map<GURL, std::string> responses;
  GURL URL = web::test::HttpServer::MakeUrl("http://example");
  responses[URL] =
      "<head><title>Example website</title></head>"
      "<body>You are here.</body>";
  web::test::SetUpSimpleHttpServer(responses);
  [ChromeEarlGrey loadURL:URL];
  chrome_test_util::SetBooleanUserPref(chrome_test_util::GetOriginalBrowserState(), password_manager::prefs::kWasAutoSignInFirstRunExperienceShown, true);
  chrome_test_util::SetBooleanUserPref(chrome_test_util::GetOriginalBrowserState(), password_manager::prefs::kCredentialsEnableAutosignin, true);

  password_manager::PasswordStore* password_store = IOSChromePasswordStoreFactory::GetForBrowserState(
             chrome_test_util::GetOriginalBrowserState(), ServiceAccessType::IMPLICIT_ACCESS)
      .get();
  GREYAssertTrue(password_store != nullptr, @"PasswordStore is unexpectedly null for BrowserState");

  // Store a PasswordForm representing a PasswordCredential.
  autofill::PasswordForm password_credential_form_;
  password_credential_form_.username_value = base::ASCIIToUTF16("johndoe@example.com");
  password_credential_form_.password_value = base::ASCIIToUTF16("ilovejanedoe123");
  password_credential_form_.origin = chrome_test_util::GetCurrentWebState()->GetLastCommittedURL().GetOrigin();
  password_credential_form_.signon_realm = password_credential_form_.origin.spec();
  password_credential_form_.scheme = autofill::PasswordForm::SCHEME_HTML;
  password_store->AddLogin(password_credential_form_);
}

// Tests that MDC snackbar saying "Signing is as ..." appears on auto sign-in.
- (void)testSnackBarAppearsOnAutoSignIn {
  __unsafe_unretained NSError* error = nil;
  NSString* result = chrome_test_util::ExecuteJavaScript(@"typeof navigator.credentials.get({password: true})", &error);
  DLOG(ERROR) << base::SysNSStringToUTF16(result);
  GREYAssertTrue([result isEqual:@"object"], @"Unexpected error occurred when executing JavaScript.");
  GREYAssertTrue(!error, @"Unexpected error occurred when executing JavaScript.");

  id<GREYMatcher> snackbar_matcher =
      chrome_test_util::ButtonWithAccessibilityLabel(
          @"Signing in as johndoe@example.com");
  // Wait for snackbar to appear.
  ConditionBlock wait_for_appearance = ^{
    NSError* error = nil;
    [[EarlGrey selectElementWithMatcher:snackbar_matcher]
        assertWithMatcher:grey_notNil()
                    error:&error];
    return error == nil;
  };
  // Wait for the snackbar to disappear.
  ConditionBlock wait_for_disappearance = ^{
    NSError* error = nil;
    [[EarlGrey selectElementWithMatcher:snackbar_matcher]
        assertWithMatcher:grey_nil()
                    error:&error];
    return error == nil;
  };
  GREYAssert(testing::WaitUntilConditionOrTimeout(kSnackbarAppearanceTimeout, wait_for_appearance), @"Snackbar did not appear");
  GREYAssert(testing::WaitUntilConditionOrTimeout(kSnackbarDisappearanceTimeout, wait_for_disappearance), @"Snackbar did not disappear");
}

@end
