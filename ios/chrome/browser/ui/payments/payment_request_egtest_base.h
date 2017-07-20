// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_EGTEST_BASE_H_
#define IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_EGTEST_BASE_H_

#include "ios/chrome/browser/payments/payment_request_cache.h"
#import "ios/chrome/test/earl_grey/chrome_test_case.h"

#include <string>
#include <vector>

namespace autofill {
class AutofillProfile;
class CreditCard;
class PersonalDataManager;
}  // namespace autofill

namespace web {
class WebState;
}  // namespace web

// Base class for various Payment Request related EarlGrey tests.
@interface PaymentRequestEGTestBase : ChromeTestCase

// Adds |profile| to the PersonalDataManager. Waits until it gets added.
- (void)addAutofillProfile:(const autofill::AutofillProfile&)profile;

// Adds |card| to the PersonalDataManager. Waits until it gets added.
- (void)addCreditCard:(const autofill::CreditCard&)card;

// Returns the PersonalDataManager instance for the current browser state.
- (autofill::PersonalDataManager*)personalDataManager;

// Returns the payments::PaymentRequest instances for |webState|.
- (payments::PaymentRequestCache::PaymentRequestSet&)paymentRequestsForWebState:
    (web::WebState*)webState;

- (void)expectWebViewContainsStrings:(const std::vector<std::string>&)strings;

@end

#endif  // IOS_CHROME_BROWSER_UI_PAYMENTS_PAYMENT_REQUEST_EGTEST_BASE_H_
