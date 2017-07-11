// Copyright 2016 The Chromium Authors. All rights reserved.
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

using chrome_test_util::ButtonWithAccessibilityLabelId;
using chrome_test_util::ButtonWithAccessibilityLabel;
using payment_request_util::GetNameLabelFromAutofillProfile;
using payment_request_util::GetShippingAddressLabelFromAutofillProfile;
using payment_request_util::GetPhoneNumberLabelFromAutofillProfile;
using payment_request_util::GetEmailLabelFromAutofillProfile;
using payment_request_util::GetAddressNotificationLabelFromAutofillProfile;

namespace {
// Displacement for scroll action.
const CGFloat kScrollDisplacement = 100.0;
// URL of the Payment Request test page.
const char kPaymentRequestDemoPage[] =
    "http://ios/testing/data/http_server_files/payment_request.html";

id<GREYMatcher> shippingAddressCellMatcher(autofill::AutofillProfile* profile) {
  NSString* emailLabel = nil;
  NSString* notificationLabel = nil;

  return chrome_test_util::ButtonWithAccessibilityLabel([NSString
      stringWithFormat:@"%@, %@, %@, %@, %@",
                       GetNameLabelFromAutofillProfile(*profile),
                       GetShippingAddressLabelFromAutofillProfile(*profile),
                       GetPhoneNumberLabelFromAutofillProfile(*profile),
                       emailLabel, notificationLabel]);
}

id<GREYMatcher> paymentMethodCellMatcher(autofill::CreditCard* creditCard) {
  NSString* billingAddressLabel = nil;
  NSString* notificationLabel = nil;

  return chrome_test_util::ButtonWithAccessibilityLabel([NSString
      stringWithFormat:@"%@, %@, %@, %@",
                       base::SysUTF16ToNSString(
                           creditCard->NetworkAndLastFourDigits()),
                       base::SysUTF16ToNSString(creditCard->GetRawInfo(
                           autofill::CREDIT_CARD_NAME_FULL)),
                       billingAddressLabel, notificationLabel]);
}

id<GREYMatcher> priceCellMatcher(PriceItem* item) {
  return chrome_test_util::ButtonWithAccessibilityLabel(
      [NSString stringWithFormat:@"%@, %@, %@", item.item, item.notification,
                                 item.price]);
}

id<GREYMatcher> shippingOptionCellMatcher(PaymentsTextItem* item) {
  return chrome_test_util::ButtonWithAccessibilityLabel(
      [NSString stringWithFormat:@"%@, %@", item.text, item.detailText]);
}

id<GREYMatcher> contactInfoCellMatcher(autofill::AutofillProfile* profile) {
  NSString* nameLabel = nil;
  NSString* shippingAddressLabel = nil;
  NSString* phoneNumberLabel = nil;
  NSString* notificationLabel = nil;

  return chrome_test_util::ButtonWithAccessibilityLabel(
      [NSString stringWithFormat:@"%@, %@, %@, %@, %@", nameLabel,
                                 shippingAddressLabel, phoneNumberLabel,
                                 GetEmailLabelFromAutofillProfile(*profile),
                                 notificationLabel]);
}

}  // namespace

// Various tests for Payment Request.
@interface PaymentRequestTestCase : ChromeTestCase

@end

@implementation PaymentRequestTestCase

#pragma mark - XCTest.

// Set up called once for the class.
- (void)setUp {
  [super setUp];

  autofill::PersonalDataManager* personalDataManager =
      autofill::PersonalDataManagerFactory::GetForBrowserState(
          chrome_test_util::GetOriginalBrowserState());

  autofill::AutofillProfile profile = autofill::test::GetFullProfile();
  personalDataManager->AddProfile(profile);
  personalDataManager->AddProfile(autofill::test::GetFullProfile2());

  autofill::CreditCard creditCard = autofill::test::GetCreditCard();
  creditCard.set_billing_address_id(profile.guid());
  personalDataManager->AddCreditCard(creditCard);
  personalDataManager->AddCreditCard(autofill::test::GetCreditCard2());
}

#pragma mark - Tests.

// Tests accessibility on the Payment Request summary page.
- (void)testAccessibilityOnPaymentRequestSummaryPage {
  [self openPaymentRequestPage];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Close summary page.
  [[EarlGrey selectElementWithMatcher:ButtonWithAccessibilityLabelId(
                                          IDS_ACCNAME_CANCEL)]
      performAction:grey_tap()];
}

// Tests accessibility on the Payment Request order summary page.
- (void)testAccessibilityOnPaymentRequestOrderSummaryPage {
  [self openPaymentRequestPage];

  PriceItem* item = [[PriceItem alloc] init];
  item.item = @"Donation";
  item.notification = nil;
  item.price = @"USD $55.00";

  [[EarlGrey selectElementWithMatcher:priceCellMatcher(item)]
      performAction:grey_tap()];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Go back to the payment summary page.
  [[EarlGrey
      selectElementWithMatcher:ButtonWithAccessibilityLabelId(IDS_ACCNAME_BACK)]
      performAction:grey_tap()];

  // Close summary page.
  [[EarlGrey selectElementWithMatcher:ButtonWithAccessibilityLabelId(
                                          IDS_ACCNAME_CANCEL)]
      performAction:grey_tap()];
}

// Tests accessibility on the Payment Request delivery address page.
- (void)testAccessibilityOnPaymentRequestDeliveryAddressPage {
  [self openPaymentRequestPage];

  autofill::PersonalDataManager* personalDataManager =
      autofill::PersonalDataManagerFactory::GetForBrowserState(
          chrome_test_util::GetOriginalBrowserState());
  std::vector<autofill::AutofillProfile*> profiles =
      personalDataManager->GetProfilesToSuggest();

  [[EarlGrey selectElementWithMatcher:shippingAddressCellMatcher(profiles[0])]
      performAction:grey_tap()];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Go back to the payment summary page.
  [[EarlGrey
      selectElementWithMatcher:ButtonWithAccessibilityLabelId(IDS_ACCNAME_BACK)]
      performAction:grey_tap()];

  // Close summary page.
  [[EarlGrey selectElementWithMatcher:ButtonWithAccessibilityLabelId(
                                          IDS_ACCNAME_CANCEL)]
      performAction:grey_tap()];
}

// Tests accessibility on the Payment Request delivery method page.
- (void)testAccessibilityOnPaymentRequestDeliveryMethodPage {
  [self openPaymentRequestPage];

  PaymentsTextItem* item = [[PaymentsTextItem alloc] init];
  item.text = @"Standard shipping";
  item.detailText = @"$0.00";

  [[EarlGrey selectElementWithMatcher:shippingOptionCellMatcher(item)]
      performAction:grey_tap()];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Go back to the payment summary page.
  [[EarlGrey
      selectElementWithMatcher:ButtonWithAccessibilityLabelId(IDS_ACCNAME_BACK)]
      performAction:grey_tap()];
  // Close summary page.
  [[EarlGrey selectElementWithMatcher:ButtonWithAccessibilityLabelId(
                                          IDS_ACCNAME_CANCEL)]
      performAction:grey_tap()];
}

// Tests accessibility on the Payment Request payment method page.
- (void)testAccessibilityOnPaymentRequestPaymentMethodPage {
  [self openPaymentRequestPage];

  autofill::PersonalDataManager* personalDataManager =
      autofill::PersonalDataManagerFactory::GetForBrowserState(
          chrome_test_util::GetOriginalBrowserState());
  std::vector<autofill::CreditCard*> creditCards =
      personalDataManager->GetCreditCardsToSuggest();

  [[[EarlGrey selectElementWithMatcher:paymentMethodCellMatcher(creditCards[0])]
         usingSearchAction:grey_scrollInDirection(kGREYDirectionDown,
                                                  2 * kScrollDisplacement)
      onElementWithMatcher:grey_accessibilityID(
                               kPaymentRequestCollectionViewID)]
      performAction:grey_tap()];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Go back to the payment summary page.
  [[EarlGrey
      selectElementWithMatcher:ButtonWithAccessibilityLabelId(IDS_ACCNAME_BACK)]
      performAction:grey_tap()];
  // Close summary page.
  [[EarlGrey selectElementWithMatcher:ButtonWithAccessibilityLabelId(
                                          IDS_ACCNAME_CANCEL)]
      performAction:grey_tap()];
}

// Tests accessibility on the Payment Request contact info page.
- (void)testAccessibilityOnPaymentRequestContactInfoPage {
  [self openPaymentRequestPage];

  autofill::PersonalDataManager* personalDataManager =
      autofill::PersonalDataManagerFactory::GetForBrowserState(
          chrome_test_util::GetOriginalBrowserState());
  std::vector<autofill::AutofillProfile*> profiles =
      personalDataManager->GetProfilesToSuggest();

  [[[EarlGrey selectElementWithMatcher:contactInfoCellMatcher(profiles[0])]
         usingSearchAction:grey_scrollInDirection(kGREYDirectionDown,
                                                  2 * kScrollDisplacement)
      onElementWithMatcher:grey_accessibilityID(
                               kPaymentRequestCollectionViewID)]
      performAction:grey_tap()];
  chrome_test_util::VerifyAccessibilityForCurrentScreen();

  // Go back to the payment summary page.
  [[EarlGrey
      selectElementWithMatcher:ButtonWithAccessibilityLabelId(IDS_ACCNAME_BACK)]
      performAction:grey_tap()];
  // Close summary page.
  [[EarlGrey selectElementWithMatcher:ButtonWithAccessibilityLabelId(
                                          IDS_ACCNAME_CANCEL)]
      performAction:grey_tap()];
}

#pragma mark - Helper functions.

- (void)openPaymentRequestPage {
  GURL URL = web::test::HttpServer::MakeUrl(kPaymentRequestDemoPage);
  [ChromeEarlGrey loadURL:URL];

  // Tap the buy button.
  chrome_test_util::TapWebViewElementWithId("buy");
}

@end
