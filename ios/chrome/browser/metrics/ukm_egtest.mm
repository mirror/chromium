// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <EarlGrey/EarlGrey.h>
#import <XCTest/XCTest.h>

#include "components/metrics/metrics_service.h"
#include "components/metrics_services_manager/metrics_services_manager.h"
#include "components/strings/grit/components_strings.h"
#include "components/ukm/ukm_service.h"
#include "ios/chrome/browser/application_context.h"
#include "ios/chrome/browser/metrics/ios_chrome_metrics_service_accessor.h"
#import "ios/chrome/browser/ui/authentication/signin_earlgrey_utils.h"
#import "ios/chrome/browser/ui/authentication/signin_promo_view.h"
#include "ios/chrome/grit/ios_strings.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/app/sync_test_util.h"
#import "ios/chrome/test/app/tab_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_actions.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/public/provider/chrome/browser/signin/fake_chrome_identity.h"
#import "ios/public/provider/chrome/browser/signin/fake_chrome_identity_service.h"
#import "ios/testing/wait_util.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

using chrome_test_util::AccountsSyncButton;
using chrome_test_util::ButtonWithAccessibilityLabelId;
using chrome_test_util::GetIncognitoTabCount;
using chrome_test_util::IsIncognitoMode;
using chrome_test_util::IsSyncInitialized;
using chrome_test_util::NavigationBarDoneButton;
using chrome_test_util::SecondarySignInButton;
using chrome_test_util::SettingsAccountButton;
using chrome_test_util::SettingsAccountButton;
using chrome_test_util::SignOutAccountsButton;
using chrome_test_util::SyncSwitchCell;
using chrome_test_util::TurnSyncSwitchOn;

namespace metrics {

// Helper class that provides access to UKM internals.
class UkmEGTestHelper {
 public:
  UkmEGTestHelper() {}

  static bool ukm_enabled() {
    auto* service = ukm_service();
    return service ? service->recording_enabled_ : false;
  }

  static uint64_t client_id() {
    auto* service = ukm_service();
    return service ? service->client_id_ : 0;
  }

  static bool metrics_enabled;

 private:
  static ukm::UkmService* ukm_service() {
    return GetApplicationContext()
        ->GetMetricsServicesManager()
        ->GetUkmService();
  }
};

bool UkmEGTestHelper::metrics_enabled = true;

}  // namespace metrics

namespace {

// Constant for timeout while waiting for asynchronous sync and UKM operations.
const NSTimeInterval kSyncUKMOperationsTimeout = 10.0;

void AssertSyncInitialized(bool is_initialized) {
  ConditionBlock condition = ^{
    return IsSyncInitialized() == is_initialized;
  };
  GREYAssert(testing::WaitUntilConditionOrTimeout(kSyncUKMOperationsTimeout,
                                                  condition),
             @"Failed to assert whether Sync was initialized or not.");
}

void AssertUKMEnabled(bool is_enabled) {
  ConditionBlock condition = ^{
    return metrics::UkmEGTestHelper::ukm_enabled() == is_enabled;
  };
  GREYAssert(testing::WaitUntilConditionOrTimeout(kSyncUKMOperationsTimeout,
                                                  condition),
             @"Failed to assert whether UKM was enabled or not.");
}

void OpenNewIncognitoTab() {
  NSUInteger incognito_tab_count = GetIncognitoTabCount();
  chrome_test_util::OpenNewIncognitoTab();
  ConditionBlock condition = ^bool {
    return GetIncognitoTabCount() == incognito_tab_count + 1;
  };
  GREYAssert(testing::WaitUntilConditionOrTimeout(
                 testing::kWaitForUIElementTimeout, condition),
             @"Waiting switch to incognito mode.");

  GREYAssert(IsIncognitoMode(), @"Failed to switch to incognito mode.");
}

void CloseAllIncognitoTabs() {
  GREYAssert(chrome_test_util::CloseAllIncognitoTabs(), @"Tabs did not close");
  ConditionBlock condition = ^bool {
    return GetIncognitoTabCount() == 0;
  };
  GREYAssert(testing::WaitUntilConditionOrTimeout(
                 testing::kWaitForUIElementTimeout, condition),
             @"Waiting for all the incognito tabs to close.");

  GREYAssert(!IsIncognitoMode(), @"Failed to switch to normal mode.");
}

// Signs in to sync.
void SignIn() {
  ChromeIdentity* identity = [SigninEarlGreyUtils fakeIdentity1];
  ios::FakeChromeIdentityService::GetInstanceFromChromeProvider()->AddIdentity(
      identity);

  [ChromeEarlGreyUI openSettingsMenu];
  [ChromeEarlGreyUI tapSettingsMenuButton:SecondarySignInButton()];
  [ChromeEarlGreyUI signInToIdentityByEmail:identity.userEmail];
  [ChromeEarlGreyUI confirmSigninConfirmationDialog];
  [[EarlGrey selectElementWithMatcher:NavigationBarDoneButton()]
      performAction:grey_tap()];

  [SigninEarlGreyUtils assertSignedInWithIdentity:identity];
}

// Signs in to sync by tapping the sign-in promo view.
void SignInWithPromo() {
  [ChromeEarlGreyUI openSettingsMenu];
  [SigninEarlGreyUtils
      checkSigninPromoVisibleWithMode:SigninPromoViewModeWarmState];
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kSigninPromoPrimaryButtonId)]
      performAction:grey_tap()];
  [ChromeEarlGreyUI confirmSigninConfirmationDialog];
  [[EarlGrey selectElementWithMatcher:NavigationBarDoneButton()]
      performAction:grey_tap()];

  [SigninEarlGreyUtils
      assertSignedInWithIdentity:[SigninEarlGreyUtils fakeIdentity1]];
}

// Signs out of sync.
void SignOut() {
  [ChromeEarlGreyUI openSettingsMenu];
  [[EarlGrey selectElementWithMatcher:SettingsAccountButton()]
      performAction:grey_tap()];
  [ChromeEarlGreyUI tapAccountsMenuButton:SignOutAccountsButton()];
  [[EarlGrey selectElementWithMatcher:
                 ButtonWithAccessibilityLabelId(
                     IDS_IOS_DISCONNECT_DIALOG_CONTINUE_BUTTON_MOBILE)]
      performAction:grey_tap()];
  [[EarlGrey selectElementWithMatcher:NavigationBarDoneButton()]
      performAction:grey_tap()];

  [SigninEarlGreyUtils assertSignedOut];
}

}  // namespace

// UKM tests.
@interface UKMTestCase : ChromeTestCase

@end

@implementation UKMTestCase

+ (void)setUp {
  [super setUp];
  if (!base::FeatureList::IsEnabled(ukm::kUkmFeature)) {
    // ukm::kUkmFeature feature is not enabled. You need to pass
    // --enable-features=Ukm command line argument in order to run this test.
    DCHECK(false);
  }
}

- (void)setUp {
  [super setUp];

  AssertUKMEnabled(false);

  // Enable sync.
  SignIn();
  AssertSyncInitialized(true);

  // Grant metrics consent and update MetricsServicesManager.
  IOSChromeMetricsServiceAccessor::SetMetricsAndCrashReportingForTesting(
      &metrics::UkmEGTestHelper::metrics_enabled);
  metrics::UkmEGTestHelper::metrics_enabled = true;
  GetApplicationContext()->GetMetricsServicesManager()->UpdateUploadPermissions(
      true);

  AssertUKMEnabled(true);
}

- (void)tearDown {
  // Disable metrics recording.
  IOSChromeMetricsServiceAccessor::SetMetricsAndCrashReportingForTesting(
      nullptr);

  // Disable sync.
  SignOut();

  AssertUKMEnabled(false);

  [super tearDown];
}

// Make sure that UKM is disabled while an incognito tab is open.
- (void)testIncognito {
  uint64_t original_client_id = metrics::UkmEGTestHelper::client_id();

  OpenNewIncognitoTab();

  AssertUKMEnabled(false);

  CloseAllIncognitoTabs();

  AssertUKMEnabled(true);
  // Client ID should not have been reset.
  GREYAssert(original_client_id == metrics::UkmEGTestHelper::client_id(),
             @"Client ID was reset.");
}

// Make sure that UKM is disabled when sync is not enabled.
- (void)testNoSync {
  uint64_t original_client_id = metrics::UkmEGTestHelper::client_id();

  SignOut();

  AssertUKMEnabled(false);

  SignInWithPromo();

  AssertUKMEnabled(true);
  // Client ID should not have been reset.
  GREYAssert(original_client_id == metrics::UkmEGTestHelper::client_id(),
             @"Client ID was reset.");
}

// Make sure that UKM is disabled when sync is disabled.
- (void)testDisableSync {
  uint64_t original_client_id = metrics::UkmEGTestHelper::client_id();

  [ChromeEarlGreyUI openSettingsMenu];
  // Open accounts settings, then sync settings.
  [[EarlGrey selectElementWithMatcher:SettingsAccountButton()]
      performAction:grey_tap()];
  [[EarlGrey selectElementWithMatcher:AccountsSyncButton()]
      performAction:grey_tap()];
  // Toggle "Sync Everything" then "History" switches off.
  [[EarlGrey selectElementWithMatcher:SyncSwitchCell(
                                          l10n_util::GetNSString(
                                              IDS_IOS_SYNC_EVERYTHING_TITLE),
                                          YES)]
      performAction:TurnSyncSwitchOn(NO)];
  [[EarlGrey
      selectElementWithMatcher:SyncSwitchCell(l10n_util::GetNSString(
                                                  IDS_SYNC_DATATYPE_TYPED_URLS),
                                              YES)]
      performAction:TurnSyncSwitchOn(NO)];

  AssertUKMEnabled(false);

  // Toggle "History" then "Sync Everything" switches on.
  [[EarlGrey
      selectElementWithMatcher:SyncSwitchCell(l10n_util::GetNSString(
                                                  IDS_SYNC_DATATYPE_TYPED_URLS),
                                              NO)]
      performAction:TurnSyncSwitchOn(YES)];
  [[EarlGrey selectElementWithMatcher:SyncSwitchCell(
                                          l10n_util::GetNSString(
                                              IDS_IOS_SYNC_EVERYTHING_TITLE),
                                          NO)]
      performAction:TurnSyncSwitchOn(YES)];

  AssertUKMEnabled(true);
  // Client ID should have been reset.
  GREYAssert(original_client_id != metrics::UkmEGTestHelper::client_id(),
             @"Client ID was not reset.");

  [[EarlGrey selectElementWithMatcher:NavigationBarDoneButton()]
      performAction:grey_tap()];
}

// Make sure that UKM is disabled when metrics consent is revoked.
- (void)testNoConsent {
  uint64_t original_client_id = metrics::UkmEGTestHelper::client_id();

  // Revoke metrics consent and update MetricsServicesManager.
  metrics::UkmEGTestHelper::metrics_enabled = false;
  GetApplicationContext()->GetMetricsServicesManager()->UpdateUploadPermissions(
      true);

  AssertUKMEnabled(false);

  // Grant metrics consent and update MetricsServicesManager.
  metrics::UkmEGTestHelper::metrics_enabled = true;
  GetApplicationContext()->GetMetricsServicesManager()->UpdateUploadPermissions(
      true);

  AssertUKMEnabled(true);
  // Client ID should have been reset.
  GREYAssert(original_client_id != metrics::UkmEGTestHelper::client_id(),
             @"Client ID was not reset.");
}

@end
