// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/strings/grit/components_strings.h"
#import "ios/chrome/browser/payments/payment_request_cache.h"
#import "ios/chrome/browser/ui/payments/payment_request_egtest_base.h"
#import "ios/chrome/browser/ui/payments/payment_request_error_view_controller.h"
#import "ios/chrome/browser/ui/payments/payment_request_view_controller.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/web/public/test/http_server/http_server.h"
#import "ios/web/public/test/web_view_interaction_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
using chrome_test_util::ButtonWithAccessibilityLabel;
using chrome_test_util::ButtonWithAccessibilityLabelId;
using chrome_test_util::GetCurrentWebState;

// URLs of the test pages.
const char kMultipleRequestsPage[] =
    "http://components/test/data/payments/"
    "payment_request_multiple_requests.html";
const char kAbortPage[] =
    "http://components/test/data/payments/payment_request_abort_test.html";
const char kNoShippingPage[] =
    "http://components/test/data/payments/"
    "payment_request_no_shipping_test.html";

}  // namepsace

@interface PaymentRequestCacheEGTest : PaymentRequestEGTestBase

@end

@implementation PaymentRequestCacheEGTest

#pragma mark - Tests

// Tests that the page can create multiple PaymentRequest objects.
- (void)testMultipleRequests {
  [ChromeEarlGrey
      loadURL:web::test::HttpServer::MakeUrl(kMultipleRequestsPage)];

  const payments::PaymentRequestCache::PaymentRequestSet& payment_requests =
      [self paymentRequestsForWebState:GetCurrentWebState()];
  EXPECT_EQ(5U, payment_requests.size());
}

@end

@interface PaymentRequestOpenAndCloseEGTest : PaymentRequestEGTestBase

@end

@implementation PaymentRequestOpenAndCloseEGTest

#pragma mark - Tests

// Tests that navigating to a URL closes the UI.
- (void)testOpenAndNavigateToURL {
  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(kAbortPage)];

  // Tap the buy button.
  web::test::TapWebViewElementWithId(GetCurrentWebState(), "buy");

  // Confirm that the Payment Request UI is showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_notNil()];

  // Load a URL.
  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(kAbortPage)];

  // Confirm that the Payment Request UI is not showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_nil()];
}

// Tests that reloading the page closes the UI.
- (void)testOpenAndReload {
  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(kAbortPage)];

  // Tap the buy button.
  web::test::TapWebViewElementWithId(GetCurrentWebState(), "buy");

  // Confirm that the Payment Request UI is showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_notNil()];

  // Reload the page.
  [ChromeEarlGrey reload];

  // Confirm that the Payment Request UI is not showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_nil()];
}

// Tests that navigating to the previous page closes the UI.
- (void)testOpenAndNavigateBack {
  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(kAbortPage)];

  // Tap the buy button.
  web::test::TapWebViewElementWithId(GetCurrentWebState(), "buy");

  // Confirm that the Payment Request UI is showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_notNil()];

  // Navigate to the previous page.
  [ChromeEarlGrey goBack];

  // Confirm that the Payment Request UI is not showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_nil()];
}

// Tests that tapping the cancel button closes the UI and rejects the Promise
// returned by show() with the appropriate error message.
- (void)testOpenAndCancel {
  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(kAbortPage)];

  // Tap the buy button.
  web::test::TapWebViewElementWithId(GetCurrentWebState(), "buy");

  // Confirm that the Payment Request UI is showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_notNil()];

  // Tap the Cancel button.
  [[EarlGrey selectElementWithMatcher:ButtonWithAccessibilityLabelId(
                                          IDS_ACCNAME_CANCEL)]
      performAction:grey_tap()];

  // Confirm that the Payment Request UI is not showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_nil()];

  web::test::WaitForWebViewContainingTextOrTimeout(GetCurrentWebState(),
                                                   @"Request cancelled");
}

// Tests that tapping the link to Chrome Settings closes the UI, rejects the
// Promise returned by show() with the appropriate error message, and displays
// the Autofill Settings UI.
- (void)testOpenAndNavigateToSettings {
  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(kAbortPage)];

  // Tap the buy button.
  web::test::TapWebViewElementWithId(GetCurrentWebState(), "buy");

  // Confirm that the Payment Request UI is showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_notNil()];

  // Tap the Settings link.
  [[EarlGrey selectElementWithMatcher:ButtonWithAccessibilityLabel(@"Settings")]
      performAction:grey_tap()];

  // Confirm that the Payment Request UI is not showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_nil()];

  // Confirm that the Autofill Settings UI is showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          @"kAutofillCollectionViewId")]
      assertWithMatcher:grey_notNil()];

  web::test::WaitForWebViewContainingTextOrTimeout(GetCurrentWebState(),
                                                   @"Request cancelled");
}

// Tests that tapping the pay button closes the UI and send the appropriate
// response.
- (void)testOpenAndPay {
  autofill::AutofillProfile profile = autofill::test::GetFullProfile();
  [self addAutofillProfile:profile];
  autofill::CreditCard card = autofill::test::GetCreditCard();
  card.set_billing_address_id(profile.guid());
  [self addCreditCard:card];

  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(kNoShippingPage)];

  // Tap the buy button.
  web::test::TapWebViewElementWithId(GetCurrentWebState(), "buy");

  // Confirm that the Payment Request UI is showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_notNil()];

  // Tap the Buy button.
  [[EarlGrey selectElementWithMatcher:ButtonWithAccessibilityLabelId(
                                          IDS_PAYMENTS_PAY_BUTTON)]
      performAction:grey_tap()];

  // Confirm that the Payment Request UI is not showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_nil()];

  [self
      expectWebViewContainsStrings:
          {base::UTF16ToUTF8(card.GetRawInfo(autofill::CREDIT_CARD_NUMBER)),
           base::UTF16ToUTF8(card.GetRawInfo(autofill::CREDIT_CARD_NAME_FULL)),
           base::UTF16ToUTF8(card.GetRawInfo(autofill::CREDIT_CARD_EXP_MONTH)),
           base::UTF16ToUTF8(
               card.GetRawInfo(autofill::CREDIT_CARD_EXP_4_DIGIT_YEAR))}];
  [self
      expectWebViewContainsStrings:
          {base::UTF16ToUTF8(profile.GetRawInfo(autofill::NAME_FIRST)),
           base::UTF16ToUTF8(profile.GetRawInfo(autofill::NAME_LAST)),
           base::UTF16ToUTF8(profile.GetRawInfo(autofill::ADDRESS_HOME_LINE1)),
           base::UTF16ToUTF8(profile.GetRawInfo(autofill::ADDRESS_HOME_LINE2)),
           base::UTF16ToUTF8(
               profile.GetRawInfo(autofill::ADDRESS_HOME_COUNTRY)),
           base::UTF16ToUTF8(profile.GetRawInfo(autofill::ADDRESS_HOME_ZIP)),
           base::UTF16ToUTF8(profile.GetRawInfo(autofill::ADDRESS_HOME_CITY)),
           base::UTF16ToUTF8(
               profile.GetRawInfo(autofill::ADDRESS_HOME_STATE))}];
}

// Tests the use of the abort() JS API.
- (void)testAbortByWebsite {
  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(kAbortPage)];

  // Tap the buy button.
  web::test::TapWebViewElementWithId(GetCurrentWebState(), "buy");

  // Confirm that the Payment Request UI is showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_notNil()];

  // Tap the abort button.
  web::test::TapWebViewElementWithId(GetCurrentWebState(), "abort");

  // Confirm that the error UI is showing.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestErrorCollectionViewID)]
      assertWithMatcher:grey_notNil()];

  // Confirm the error.
  [[EarlGrey
      selectElementWithMatcher:ButtonWithAccessibilityLabelId(IDS_ACCNAME_OK)]
      performAction:grey_tap()];

  // Confirm that the Payment Request UI is not showing any longer.
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kPaymentRequestCollectionViewID)]
      assertWithMatcher:grey_nil()];

  [self expectWebViewContainsStrings:{"The website has aborted the payment",
                                      "Aborted"}];
}

- (void)testAbortUnsuccessfulAfterCVCPromptShown {
  // TODO(crbug.com/602666): test that abort() should fail if CVC unmask prompt
  // UI is showing.
}

@end
