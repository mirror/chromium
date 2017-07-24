// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card.h"
#import "ios/chrome/browser/ui/payments/payment_request_egtest_base.h"
#include "ios/chrome/browser/ui/tools_menu/tools_menu_constants.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey_ui.h"
#import "ios/web/public/test/http_server/http_server.h"
#include "testing/gtest/include/gtest/gtest.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// URLs of the test pages.
const char kCanMakePaymentPage[] =
    "https://components/test/data/payments/"
    "payment_request_can_make_payment_query_test.html";
const char kCanMakePaymentCreditCardPage[] =
    "https://components/test/data/payments/"
    "payment_request_can_make_payment_query_cc_test.html";
const char kCanMakePaymentMethodIdentifierPage[] =
    "https://components/test/data/payments/"
    "payment_request_payment_method_identifier_test.html";

}  // namepsace

@interface PaymentRequestCanMakePaymentEGTest : PaymentRequestEGTestBase

@end

@implementation PaymentRequestCanMakePaymentEGTest

#pragma mark - Tests

// Tests canMakePayment() when visa is required and user has a visa instrument.
- (void)testCanMakePaymentIsSupported {
  [self addCreditCard:autofill::test::GetCreditCard()];  // visa.

  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(kCanMakePaymentPage)];

  // Query visa payment method.
  [self tapWebViewElementWithId:"buy"];

  [self expectWebViewContainsStrings:{"true"}];
}

// Tests canMakePayment() when visa is required, user has a visa instrument, and
// user is in incognito mode.
- (void)testCanMakePaymentIsSupportedInIncognitoMode {
  [self addCreditCard:autofill::test::GetCreditCard()];  // visa.

  // Open an Incognito tab.
  [ChromeEarlGreyUI openToolsMenu];
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kToolsMenuNewIncognitoTabId)]
      performAction:grey_tap()];
  [ChromeEarlGrey waitForIncognitoTabCount:1];

  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(kCanMakePaymentPage)];

  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(kCanMakePaymentPage)];

  // Query payment method.
  [self tapWebViewElementWithId:"buy"];

  [self expectWebViewContainsStrings:{"true"}];
}

// Tests canMakePayment() when visa is required, but user doesn't have one.
- (void)testCanMakePaymentIsNotSupported {
  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(kCanMakePaymentPage)];

  // Query payment method.
  [self tapWebViewElementWithId:"buy"];

  [self expectWebViewContainsStrings:{"false"}];
}

// Tests canMakePayment() when visa is required, user doesn't have a visa
// instrument and the user is in incognito mode. In this case canMakePayment()
// returns true.
- (void)testCanMakePaymentIsNotSupportedInIncognitoMode {
  // Open an Incognito tab.
  [ChromeEarlGreyUI openToolsMenu];
  [[EarlGrey selectElementWithMatcher:grey_accessibilityID(
                                          kToolsMenuNewIncognitoTabId)]
      performAction:grey_tap()];
  [ChromeEarlGrey waitForIncognitoTabCount:1];

  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(kCanMakePaymentPage)];

  // Query payment method.
  [self tapWebViewElementWithId:"buy"];

  [self expectWebViewContainsStrings:{"true"}];
}

@end

@interface PaymentRequestCanMakePaymentExceedsQueryQuotaEGTest
    : PaymentRequestEGTestBase

@end

@implementation PaymentRequestCanMakePaymentExceedsQueryQuotaEGTest

// Tests canMakePayment() exceeds query quota when different payment methods are
// queried one after another.
- (void)testCanMakePaymentExceedsQueryQuota {
  [ChromeEarlGrey
      loadURL:web::test::HttpServer::MakeUrl(kCanMakePaymentCreditCardPage)];

  // Query visa payment method.
  [self tapWebViewElementWithId:"buy"];

  // User does not have a visa card.
  [self expectWebViewContainsStrings:{"false"}];

  // Query Mastercard payment method.
  [self tapWebViewElementWithId:"other-buy"];

  // Query quota exceeded.
  [self
      expectWebViewContainsStrings:
          {"NotAllowedError", "Not allowed to check whether can make payment"}];

  [self addCreditCard:autofill::test::GetCreditCard()];  // visa.

  // Query visa payment method.
  [self tapWebViewElementWithId:"buy"];

  // User has a visa card. While the query is cached, result is always fresh.
  [self expectWebViewContainsStrings:{"true"}];

  // Query Mastercard payment method.
  [self tapWebViewElementWithId:"other-buy"];

  // Query quota exceeded.
  [self
      expectWebViewContainsStrings:
          {"NotAllowedError", "Not allowed to check whether can make payment"}];
}

@end

@interface PaymentRequestCanMakePaymentExceedsQueryQuotaBasicaCardEGTest
    : PaymentRequestEGTestBase

@end

@implementation PaymentRequestCanMakePaymentExceedsQueryQuotaBasicaCardEGTest

#pragma mark - Tests

// Tests canMakePayment() exceeds query quota when different payment methods are
// queried one after another.
- (void)testCanMakePaymentExceedsQueryQuotaBasicaCard {
  [ChromeEarlGrey loadURL:web::test::HttpServer::MakeUrl(
                              kCanMakePaymentMethodIdentifierPage)];

  // Query basic-card payment method with "supportedNetworks": ["visa"] in the
  // payment method specific data.
  [self tapWebViewElementWithId:"checkBasicVisa"];

  // User does not have a visa card.
  [self expectWebViewContainsStrings:{"false"}];

  // Query basic-card payment method without "supportedNetworks" parameter.
  [self tapWebViewElementWithId:"checkBasicCard"];

  // Query quota exceeded.
  [self
      expectWebViewContainsStrings:
          {"NotAllowedError", "Not allowed to check whether can make payment"}];

  [self addCreditCard:autofill::test::GetCreditCard()];  // visa.

  // Query basic-card payment method with "supportedNetworks": ["visa"] in the
  // payment method specific data.
  [self tapWebViewElementWithId:"checkBasicVisa"];

  // User has a visa card. While the query is cached, result is always fresh.
  [self expectWebViewContainsStrings:{"true"}];

  // Query basic-card payment method without "supportedNetworks" parameter.
  [self tapWebViewElementWithId:"checkBasicCard"];

  // Query quota exceeded.
  [self
      expectWebViewContainsStrings:
          {"NotAllowedError", "Not allowed to check whether can make payment"}];
}

@end
