// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/credential_manager.h"

#import <EarlGrey/EarlGrey.h>
#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>

#include "base/strings/utf_string_conversions.h"
#include "components/keyed_service/core/service_access_type.h"
#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "ios/chrome/browser/passwords/credential_manager_features.h"
#include "ios/chrome/browser/passwords/ios_chrome_password_store_factory.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "ios/web/public/test/http_server/http_server_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

// Time that is allowed to elapse between calling API method from JS and UI
// being displayed.
const CFTimeInterval kAppearanceTimeout = 5;
// Notification should be displayed for 3 seconds. 3.5 is safer timeout to check
// and also smaller than 4.
const CFTimeInterval kDisappearanceTimeout = 3.5;
// Provides basic response for example page.
std::unique_ptr<net::test_server::HttpResponse> StandardResponse(
    const net::test_server::HttpRequest& request) {
  std::unique_ptr<net::test_server::BasicHttpResponse> http_response =
      base::MakeUnique<net::test_server::BasicHttpResponse>();
  http_response->set_code(net::HTTP_OK);
  http_response->set_content(
      "<head><title>Example website</title></head>"
      "<body>You are here.</body>");
  return std::move(http_response);
}

}  // namespace

// This class tests that a notification saying "Signing in as ..." is displayed
// on auto sign-in and disappears after 3 seconds.
@interface CredentialManagerAutoSigninTestCase : ChromeTestCase

@end

@implementation CredentialManagerAutoSigninTestCase

- (void)setUp {
  [super setUp];

  if (!base::FeatureList::IsEnabled(features::kCredentialManager)) {
    // CredentialManager feature is not enabled.
    // You have to pass --enable-features=CredentialManager command line
    // argument in order to run this test.
    GREYFail(
        @"CredentialManager feature is not enabled. Please re-run the test "
        @"with --enable-features=CredentialManager.");
  }

  // Set up server.
  self.testServer->RegisterRequestHandler(base::Bind(&StandardResponse));
  GREYAssertTrue(self.testServer->Start(), @"Test server failed to start.");
}

- (void)tearDown {
  scoped_refptr<password_manager::PasswordStore> passwordStore =
      IOSChromePasswordStoreFactory::GetForBrowserState(
          chrome_test_util::GetOriginalBrowserState(),
          ServiceAccessType::IMPLICIT_ACCESS)
          .get();
  // Remove Credentials stored during executing the test.
  passwordStore->RemoveLoginsCreatedBetween(base::Time(), base::Time::Now(),
                                            base::Closure());
  [super tearDown];
}

// Tests that notification saying "Signing is as ..." appears on auto sign-in.
- (void)testNotificationAppearsOnAutoSignIn {
  // Loads simple page. It is on localhost so it is considered a secure context.
  const GURL URL = self.testServer->GetURL("/example");
  [ChromeEarlGrey loadURL:URL];

  // Sets required preferences to true.
  chrome_test_util::SetBooleanUserPref(
      chrome_test_util::GetOriginalBrowserState(),
      password_manager::prefs::kWasAutoSignInFirstRunExperienceShown, true);
  chrome_test_util::SetBooleanUserPref(
      chrome_test_util::GetOriginalBrowserState(),
      password_manager::prefs::kCredentialsEnableAutosignin, true);

  // Obtain a PasswordStore. IMPLICIT_ACCESS flag is used because operations
  // performed by Credential Manager API are incompatible with incognito mode.
  scoped_refptr<password_manager::PasswordStore> passwordStore =
      IOSChromePasswordStoreFactory::GetForBrowserState(
          chrome_test_util::GetOriginalBrowserState(),
          ServiceAccessType::IMPLICIT_ACCESS)
          .get();
  GREYAssertTrue(passwordStore != nullptr,
                 @"PasswordStore is unexpectedly null for BrowserState");

  // Store a PasswordForm representing a PasswordCredential.
  autofill::PasswordForm passwordCredentialForm;
  passwordCredentialForm.username_value =
      base::ASCIIToUTF16("johndoe@example.com");
  passwordCredentialForm.password_value = base::ASCIIToUTF16("ilovejanedoe123");
  passwordCredentialForm.origin =
      chrome_test_util::GetCurrentWebState()->GetLastCommittedURL().GetOrigin();
  passwordCredentialForm.signon_realm = passwordCredentialForm.origin.spec();
  passwordCredentialForm.scheme = autofill::PasswordForm::SCHEME_HTML;
  passwordStore->AddLogin(passwordCredentialForm);

  // Call get() from JavaScript.
  __unsafe_unretained NSError* error = nil;
  NSString* result = chrome_test_util::ExecuteJavaScript(
      @"typeof navigator.credentials.get({password: true})", &error);
  GREYAssertTrue([result isEqual:@"object"],
                 @"Unexpected error occurred when executing JavaScript.");
  GREYAssertTrue(!error,
                 @"Unexpected error occurred when executing JavaScript.");

  // Matches the UILabel by its accessibilityLabel.
  id<GREYMatcher> matcher =
      grey_allOf(grey_accessibilityLabel(@"Signing in as johndoe@example.com"),
                 grey_accessibilityTrait(UIAccessibilityTraitStaticText), nil);
  // Wait for notification to appear.
  ConditionBlock waitForAppearance = ^{
    NSError* error = nil;
    [[EarlGrey selectElementWithMatcher:matcher] assertWithMatcher:grey_notNil()
                                                             error:&error];
    return error == nil;
  };
  // Gives some time for the notification to appear.
  GREYAssert(testing::WaitUntilConditionOrTimeout(kAppearanceTimeout,
                                                  waitForAppearance),
             @"Notification did not appear");
  // Wait for the notification to disappear.
  ConditionBlock waitForDisppearance = ^{
    NSError* error = nil;
    [[EarlGrey selectElementWithMatcher:matcher] assertWithMatcher:grey_nil()
                                                             error:&error];
    return error == nil;
  };
  // Ensures that notification disappears after time limit.
  GREYAssert(testing::WaitUntilConditionOrTimeout(kDisappearanceTimeout,
                                                  waitForDisppearance),
             @"Notification did not disappear");
}

@end
