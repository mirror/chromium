// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "chrome/browser/ui/views/payments/payment_request_browsertest_base.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card.h"
#include "content/public/test/browser_test_utils.h"

namespace payments {

constexpr auto CREDIT = ::autofill::CreditCard::CardType::CARD_TYPE_CREDIT;
constexpr auto DEBIT = ::autofill::CreditCard::CardType::CARD_TYPE_DEBIT;
constexpr auto PREPAID = ::autofill::CreditCard::CardType::CARD_TYPE_PREPAID;
constexpr auto UNKNOWN = ::autofill::CreditCard::CardType::CARD_TYPE_UNKNOWN;

// Tests for a merchant that requests a debit card.
class PaymentRequestDebitTest : public PaymentRequestBrowserTestBase {
 protected:
  PaymentRequestDebitTest()
      : PaymentRequestBrowserTestBase("/payment_request_debit_test.html") {}

  void AddCardWithType(autofill::CreditCard::CardType card_type) {
    if (billing_address_id_.empty()) {
      autofill::AutofillProfile billing_address =
          autofill::test::GetFullProfile();
      billing_address_id_ = billing_address.guid();
      AddAutofillProfile(billing_address);
    }

    autofill::CreditCard card = autofill::test::GetMaskedServerCard();
    card.set_card_type(card_type);
    card.set_billing_address_id(billing_address_id_);
    AddServerCard(card);
  }

  void CallCanMakePayment() {
    ResetEventObserver(DialogEvent::CAN_MAKE_PAYMENT_CALLED);
    ASSERT_TRUE(
        content::ExecuteScript(GetActiveWebContents(), "canMakePayment();"));
    WaitForObservedEvent();
  }

 private:
  std::string billing_address_id_;

  DISALLOW_COPY_AND_ASSIGN(PaymentRequestDebitTest);
};

IN_PROC_BROWSER_TEST_F(PaymentRequestDebitTest, CanMakePaymentWithDebitCard) {
  AddCardWithType(DEBIT);
  CallCanMakePayment();
  ExpectBodyContains({"true"});
}

IN_PROC_BROWSER_TEST_F(PaymentRequestDebitTest,
                       CanMakePaymentWithUnknownCardType) {
  AddCardWithType(UNKNOWN);
  CallCanMakePayment();
  ExpectBodyContains({"true"});
}

IN_PROC_BROWSER_TEST_F(PaymentRequestDebitTest,
                       CannotMakePaymentWithCreditAndPrepaidCard) {
  AddCardWithType(CREDIT);
  AddCardWithType(PREPAID);
  CallCanMakePayment();
  ExpectBodyContains({"false"});
}

IN_PROC_BROWSER_TEST_F(PaymentRequestDebitTest, DebitCardIsPreselected) {
  AddCardWithType(DEBIT);
  CallCanMakePayment();
  InvokePaymentRequestUI();
  EXPECT_TRUE(IsPayButtonEnabled());
  ClickOnCancel();
}

IN_PROC_BROWSER_TEST_F(PaymentRequestDebitTest,
                       UnknownCardTypeIsNotPreselected) {
  AddCardWithType(UNKNOWN);
  InvokePaymentRequestUI();
  EXPECT_FALSE(IsPayButtonEnabled());
  ClickOnCancel();
}

}  // namespace payments
