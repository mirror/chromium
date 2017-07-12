// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <XCTest/XCTest.h>

#include "base/strings/sys_string_conversions.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/strings/grit/components_strings.h"
#include "ios/chrome/browser/autofill/personal_data_manager_factory.h"
#include "ios/chrome/browser/payments/payment_request_util.h"
#import "ios/chrome/browser/ui/payments/cells/payments_text_item.h"
#import "ios/chrome/browser/ui/payments/cells/price_item.h"
#import "ios/chrome/browser/ui/payments/payment_request_view_controller.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/chrome/test/app/web_view_interaction_test_util.h"
#import "ios/chrome/test/earl_grey/accessibility_util.h"
#import "ios/chrome/test/earl_grey/chrome_earl_grey.h"
#import "ios/chrome/test/earl_grey/chrome_matchers.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"
#import "ios/web/public/test/http_server/http_server.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
using ::chrome_test_util::ButtonWithAccessibilityLabel;
using ::chrome_test_util::ButtonWithAccessibilityLabelId;
using ::payment_request_util::GetAddressNotificationLabelFromAutofillProfile;
using ::payment_request_util::GetEmailLabelFromAutofillProfile;
using ::payment_request_util::GetNameLabelFromAutofillProfile;
using ::payment_request_util::GetPhoneNumberLabelFromAutofillProfile;
using ::payment_request_util::GetShippingAddressLabelFromAutofillProfile;

// Displacement for scroll action.
const CGFloat kScrollDisplacement = 100.0;
// URL of the Payment Request test page.
const char kPaymentRequestDemoPage[] =
    "http://ios/testing/data/http_server_files/payment_request.html";

id<GREYMatcher> ShippingAddressCellMatcher(NSString* nameLabel,
                                           NSString* addressLabel,
                                           NSString* phoneNumberLabel,
                                           NSString* emailLabel,
                                           NSString* notificationLabel) {
  return chrome_test_util::ButtonWithAccessibilityLabel([NSString
      stringWithFormat:@"%@, %@, %@, %@, %@", nameLabel, addressLabel,
                       phoneNumberLabel, emailLabel, notificationLabel]);
}

id<GREYMatcher> PaymentMethodCellMatcher(NSString* mainLabel,
                                         NSString* subLabel,
                                         NSString* billingAddressLabel,
                                         NSString* notificationLabel) {
  return chrome_test_util::ButtonWithAccessibilityLabel(
      [NSString stringWithFormat:@"%@, %@, %@, %@", mainLabel, subLabel,
                                 billingAddressLabel, notificationLabel]);
}

id<GREYMatcher> PriceCellMatcher(NSString* mainLabel,
                                 NSString* notificationLabel,
                                 NSString* priceLabel) {
  return chrome_test_util::ButtonWithAccessibilityLabel(
      [NSString stringWithFormat:@"%@, %@, %@", mainLabel, notificationLabel,
                                 priceLabel]);
}

id<GREYMatcher> ShippingOptionCellMatcher(NSString* mainLabel,
                                          NSString* detailLabel) {
  return chrome_test_util::ButtonWithAccessibilityLabel(
      [NSString stringWithFormat:@"%@, %@", mainLabel, detailLabel]);
}

id<GREYMatcher> ContactInfoCellMatcher(NSString* nameLabel,
                                       NSString* addressLabel,
                                       NSString* phoneNumberLabel,
                                       NSString* emailLabel,
                                       NSString* notificationLabel) {
  return chrome_test_util::ButtonWithAccessibilityLabel([NSString
      stringWithFormat:@"%@, %@, %@, %@, %@", nameLabel, addressLabel,
                       phoneNumberLabel, emailLabel, notificationLabel]);
}

}  // namespace

// Various tests for Payment Request.
@interface PaymentRequestTestCase : ChromeTestCase

@property(nonatomic, assign) autofill::AutofillProfile profile1;

@property(nonatomic, assign) autofill::CreditCard creditCard1;

@property(nonatomic, assign) autofill::CreditCard creditCard2;

@end

@implementation PaymentRequestTestCase

@synthesize profile1 = _profile1;
@synthesize creditCard1 = _creditCard1;
@synthesize creditCard2 = _creditCard2;

#pragma mark - XCTest.

// Set up called once before each test.
- (void)setUp {
  [super setUp];

  autofill::PersonalDataManager* personalDataManager =
      autofill::PersonalDataManagerFactory::GetForBrowserState(
          chrome_test_util::GetOriginalBrowserState());

  _profile1 = autofill::test::GetFullProfile();
  _creditCard1 = autofill::test::GetCreditCard();
  _creditCard2 = autofill::test::GetCreditCard2();

  personalDataManager->AddProfile(_profile1);
  _creditCard1.set_billing_address_id(_profile1.guid());
  personalDataManager->AddCreditCard(_creditCard1);
  personalDataManager->AddCreditCard(_creditCard2);
}

#pragma mark - Tests.

// Tests accessibility on the Payment Request summary page.
- (void)testAccessibilityOnPaymentRequestSummaryPage {
  [self openPaymentRequestPage];

  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  [self closePaymentRequestPage];
}

// Tests accessibility on the Payment Request order summary page.
- (void)testAccessibilityOnPaymentRequestOrderSummaryPage {
  [self openPaymentRequestPage];

  [[EarlGrey selectElementWithMatcher:PriceCellMatcher(@"Donation", nil,
                                                       @"USD $55.00")]
      performAction:grey_tap()];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Go back to the payment summary page.
  [self goBack];

  [self closePaymentRequestPage];
}

// Tests accessibility on the Payment Request delivery address page.
- (void)testAccessibilityOnPaymentRequestDeliveryAddressPage {
  [self openPaymentRequestPage];

  [[EarlGrey
      selectElementWithMatcher:ShippingAddressCellMatcher(
                                   GetNameLabelFromAutofillProfile(_profile1),
                                   GetShippingAddressLabelFromAutofillProfile(
                                       _profile1),
                                   GetPhoneNumberLabelFromAutofillProfile(
                                       _profile1),
                                   nil, nil)] performAction:grey_tap()];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Go back to the payment summary page.
  [self goBack];

  [self closePaymentRequestPage];
}

// Tests accessibility on the Payment Request delivery method page.
- (void)testAccessibilityOnPaymentRequestDeliveryMethodPage {
  [self openPaymentRequestPage];

  [[EarlGrey selectElementWithMatcher:ShippingOptionCellMatcher(
                                          @"Standard shipping", @"$0.00")]
      performAction:grey_tap()];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Go back to the payment summary page.
  [self goBack];

  [self closePaymentRequestPage];
}

// Tests accessibility on the Payment Request payment method page.
- (void)testAccessibilityOnPaymentRequestPaymentMethodPage {
  [self openPaymentRequestPage];

  [[[EarlGrey
      selectElementWithMatcher:PaymentMethodCellMatcher(
                                   base::SysUTF16ToNSString(
                                       _creditCard1.NetworkAndLastFourDigits()),
                                   base::SysUTF16ToNSString(
                                       _creditCard1.GetRawInfo(
                                           autofill::CREDIT_CARD_NAME_FULL)),
                                   nil, nil)]
         usingSearchAction:grey_scrollInDirection(kGREYDirectionDown,
                                                  2 * kScrollDisplacement)
      onElementWithMatcher:grey_accessibilityID(
                               kPaymentRequestCollectionViewID)]
      performAction:grey_tap()];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Go back to the payment summary page.
  [self goBack];

  [self closePaymentRequestPage];
}

// Tests accessibility on the Payment Request contact info page.
- (void)testAccessibilityOnPaymentRequestContactInfoPage {
  [self openPaymentRequestPage];

  [[[EarlGrey
      selectElementWithMatcher:ContactInfoCellMatcher(
                                   GetNameLabelFromAutofillProfile(_profile1),
                                   nil,
                                   GetPhoneNumberLabelFromAutofillProfile(
                                       _profile1),
                                   GetEmailLabelFromAutofillProfile(_profile1),
                                   nil)]
         usingSearchAction:grey_scrollInDirection(kGREYDirectionDown,
                                                  2 * kScrollDisplacement)
      onElementWithMatcher:grey_accessibilityID(
                               kPaymentRequestCollectionViewID)]
      performAction:grey_tap()];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Go back to the payment summary page.
  [self goBack];

  [self closePaymentRequestPage];
}

#pragma mark - Helper functions.

- (void)openPaymentRequestPage {
  GURL URL = web::test::HttpServer::MakeUrl(kPaymentRequestDemoPage);
  [ChromeEarlGrey loadURL:URL];

  // Tap the buy button.
  chrome_test_util::TapWebViewElementWithId("buy");
}

- (void)goBack {
  [[EarlGrey selectElementWithMatcher:chrome_test_util::BackButton()]
      performAction:grey_tap()];
}

- (void)closePaymentRequestPage {
  [[EarlGrey selectElementWithMatcher:chrome_test_util::CancelButton()]
      performAction:grey_tap()];
}

@end
