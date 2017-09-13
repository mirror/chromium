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
  scoped_refptr<password_manager::PasswordStore> GetPasswordStore() {
  // ServiceAccessType governs behaviour in Incognito: only modifications with
  // EXPLICIT_ACCESS, which correspond to user's explicit gesture, succeed.
  // This test does not deal with Incognito, and should not run in Incognito
  // context. Therefore IMPLICIT_ACCESS is used to let the test fail if in
  // Incognito context.
  return IOSChromePasswordStoreFactory::GetForBrowserState(
      chrome_test_util::GetOriginalBrowserState(),
      ServiceAccessType::IMPLICIT_ACCESS);
}
const CFTimeInterval kSnackbarAppearanceTimeout = 5;
//const CFTimeInterval kSnackbarDisappearanceTimeout = 3;
// This class is used to obtain results from the PasswordStore and hence both
// check the success of store updates and ensure that store has finished
// processing.
class TestStoreConsumer : public password_manager::PasswordStoreConsumer {
 public:
  void OnGetPasswordStoreResults(
      std::vector<std::unique_ptr<autofill::PasswordForm>> obtained) override {
    obtained_ = std::move(obtained);
  }

  const std::vector<autofill::PasswordForm>& GetStoreResults() {
    results_.clear();
    ResetObtained();
    GetPasswordStore()->GetAutofillableLogins(this);
    bool responded = testing::WaitUntilConditionOrTimeout(1.0, ^bool {
      return !AreObtainedReset();
    });
    GREYAssert(responded, @"Obtaining fillable items took too long.");
    AppendObtainedToResults();
    GetPasswordStore()->GetBlacklistLogins(this);
    responded = testing::WaitUntilConditionOrTimeout(1.0, ^bool {
      return !AreObtainedReset();
    });
    GREYAssert(responded, @"Obtaining blacklisted items took too long.");
    AppendObtainedToResults();
    return results_;
  }

 private:
  // Puts |obtained_| in a known state not corresponding to any PasswordStore
  // state.
  void ResetObtained() {
    obtained_.clear();
    obtained_.emplace_back(nullptr);
  }

  // Returns true if |obtained_| are in the reset state.
  bool AreObtainedReset() { return obtained_.size() == 1 && !obtained_[0]; }

  void AppendObtainedToResults() {
    for (const auto& source : obtained_) {
      results_.emplace_back(*source);
    }
    ResetObtained();
  }

  // Temporary cache of obtained store results.
  std::vector<std::unique_ptr<autofill::PasswordForm>> obtained_;

  // Combination of fillable and blacklisted credentials from the store.
  std::vector<autofill::PasswordForm> results_;
};
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
  autofill::PasswordForm password_credential_form_;
  password_credential_form_.username_value = base::ASCIIToUTF16("johndoe@example.com");
  password_credential_form_.password_value = base::ASCIIToUTF16("ilovejanedoe123");
  password_credential_form_.origin = chrome_test_util::GetCurrentWebState()->GetLastCommittedURL().GetOrigin();
  password_credential_form_.signon_realm = password_credential_form_.origin.spec();
  password_credential_form_.scheme = autofill::PasswordForm::SCHEME_HTML;
  password_store->AddLogin(password_credential_form_);
  TestStoreConsumer consumer;
  for (const auto& result : consumer.GetStoreResults()) {
    if (result == password_credential_form_) {
      DLOG(INFO) << "Credential is properly stored";
      return;
    }
  }
  GREYFail(@"Stored form was not found in the PasswordStore results.");
}

// Tests that JavaScript is injected. TODO: remove, it was for debugging
/*- (void)testJavaScriptInjected {
  DLOG(ERROR) << "test js injected";
  __unsafe_unretained NSError* error = nil;
  NSString* result = chrome_test_util::ExecuteJavaScript(@"typeof navigator.credentials", &error);
  DLOG(ERROR) << base::SysNSStringToUTF16(result) << "\n";
  GREYAssertTrue(!error, @"Unexpected error when executing JavaScript.");
  GREYAssertTrue([result isEqual:@"object"], @"description");
}*/

// Tests that MDC snackbar saying "Signing is as ..." appears on auto sign-in.
- (void)testSnackBarAppearsOnAutoSignIn {
  __unsafe_unretained NSError* error = nil;
  chrome_test_util::ExecuteJavaScript(@"typeof navigator.credentials.get({password: true})", &error);
  GREYAssertTrue(!error, @"Unexpected error when executing JavaScript.");
  id<GREYMatcher> snackbar_matcher =
      chrome_test_util::ButtonWithAccessibilityLabel(
          @"Signing in as johndoe@example.com");
  ConditionBlock wait_for_appearance = ^{
    NSError* error = nil;
    [[EarlGrey selectElementWithMatcher:snackbar_matcher]
        assertWithMatcher:grey_notNil()
                    error:&error];
    return error == nil;
  };
  GREYAssert(testing::WaitUntilConditionOrTimeout(kSnackbarAppearanceTimeout, wait_for_appearance), @"Snackbar did not appear");
}

- (void)tearDown {
DLOG(ERROR) << "Tear down";
  [super tearDown];
}

@end
