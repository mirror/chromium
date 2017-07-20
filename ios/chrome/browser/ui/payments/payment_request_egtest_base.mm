// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/payments/payment_request_egtest_base.h"

#include <algorithm>

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "ios/chrome/browser/autofill/personal_data_manager_factory.h"
#include "ios/chrome/browser/payments/ios_payment_request_cache_factory.h"
#import "ios/chrome/test/app/chrome_test_util.h"
#import "ios/testing/wait_util.h"
#import "ios/web/public/test/web_view_interaction_test_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
// Timeout in seconds while waiting for a profile or a credit to be added to the
// PersonalDataManager.
const NSTimeInterval kPDMMaxDelaySeconds = 10.0;
}

@implementation PaymentRequestEGTestBase

std::vector<autofill::AutofillProfile> _profiles;
std::vector<autofill::CreditCard> _cards;

#pragma mark - XCTestCase

- (void)tearDown {
  // Remove the profiles and credit cards added to PersonalDataManager
  for (const auto& profile : _profiles) {
    [self personalDataManager]->RemoveByGUID(profile.guid());
  }
  _profiles.clear();

  for (const auto& card : _cards) {
    [self personalDataManager]->RemoveByGUID(card.guid());
  }
  _cards.clear();

  [super tearDown];
}

#pragma mark - Public methods

- (void)addAutofillProfile:(const autofill::AutofillProfile&)profile {
  _profiles.push_back(profile);
  size_t profile_count = [self personalDataManager]->GetProfiles().size();
  [self personalDataManager]->AddProfile(profile);
  DCHECK(testing::WaitUntilConditionOrTimeout(kPDMMaxDelaySeconds, ^bool() {
    return profile_count < [self personalDataManager]->GetProfiles().size();
  }));
}

- (void)addCreditCard:(const autofill::CreditCard&)card {
  _cards.push_back(card);
  size_t card_count = [self personalDataManager]->GetCreditCards().size();
  [self personalDataManager]->AddCreditCard(card);
  DCHECK(testing::WaitUntilConditionOrTimeout(kPDMMaxDelaySeconds, ^bool() {
    return card_count < [self personalDataManager]->GetCreditCards().size();
  }));
}

- (autofill::PersonalDataManager*)personalDataManager {
  return autofill::PersonalDataManagerFactory::GetForBrowserState(
      chrome_test_util::GetOriginalBrowserState());
}

- (payments::PaymentRequestCache::PaymentRequestSet&)paymentRequestsForWebState:
    (web::WebState*)webState {
  return payments::IOSPaymentRequestCacheFactory::GetForBrowserState(
             chrome_test_util::GetOriginalBrowserState())
      ->GetPaymentRequests(webState);
}

- (void)expectWebViewContainsStrings:(const std::vector<std::string>&)strings {
  for (const std::string& string : strings) {
    DCHECK(web::test::WaitForWebViewContainingTextOrTimeout(
        chrome_test_util::GetCurrentWebState(),
        base::SysUTF8ToNSString(string)));
  }
}

@end
