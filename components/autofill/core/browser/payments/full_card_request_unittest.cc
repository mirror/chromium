// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/payments/full_card_request.h"

#include "base/command_line.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/payments/payments_client.h"
#include "components/autofill/core/browser/personal_data_manager.h"
#include "components/autofill/core/browser/test_autofill_client.h"
#include "components/autofill/core/browser/test_autofill_driver.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {
namespace payments {

using testing::_;

// The consumer of the full card request API.
class MockDelegate : public FullCardRequest::Delegate,
                     public base::SupportsWeakPtr<MockDelegate> {
 public:
  MOCK_METHOD2(OnFullCardDetails,
               void(const CreditCard&, const base::string16&));
  MOCK_METHOD0(OnFullCardError, void());
};

// The autofill client.
class MockAutofillClient : public TestAutofillClient {
 public:
  MOCK_METHOD3(ShowUnmaskPrompt,
               void(const CreditCard&,
                    UnmaskCardReason,
                    base::WeakPtr<CardUnmaskDelegate>));
  MOCK_METHOD1(OnUnmaskVerificationResult,
               void(AutofillClient::PaymentsRpcResult));
};

// The test fixture for full card request.
class FullCardRequestTest : public testing::Test,
                            public PaymentsClientDelegate {
 public:
  FullCardRequestTest()
      : personal_data_("en_US"),
        request_context_(new net::TestURLRequestContextGetter(
            base::ThreadTaskRunnerHandle::Get())),
        payments_client_(request_context_.get(), this),
        request_(&autofill_client_, &payments_client_, &personal_data_) {
    // Silence the warning from PaymentsClient about matching sync and Payments
    // server types.
    base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
        "sync-url", "https://google.com");
  }

  ~FullCardRequestTest() override {}

  MockAutofillClient* client() { return &autofill_client_; }

  FullCardRequest* request() { return &request_; }

  CardUnmaskDelegate* ui_delegate() { return (CardUnmaskDelegate*)&request_; }

  MockDelegate* delegate() { return &delegate_; }

  // PaymentsClientDelegate:
  void OnDidGetRealPan(AutofillClient::PaymentsRpcResult result,
                       const std::string& real_pan) override {
    request_.OnDidGetRealPan(result, real_pan);
  }

 private:
  // PaymentsClientDelegate:
  IdentityProvider* GetIdentityProvider() override {
    return autofill_client_.GetIdentityProvider();
  }
  void OnDidGetUploadDetails(
      AutofillClient::PaymentsRpcResult result,
      const base::string16& context_token,
      std::unique_ptr<base::DictionaryValue> legal_message) override {
    ASSERT_TRUE(false) << "No upload details in this test";
  }
  void OnDidUploadCard(AutofillClient::PaymentsRpcResult result) override {
    ASSERT_TRUE(false) << "No card uploads in this test.";
  }

  base::MessageLoop message_loop_;
  PersonalDataManager personal_data_;
  MockAutofillClient autofill_client_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_;
  PaymentsClient payments_client_;
  MockDelegate delegate_;
  FullCardRequest request_;

  DISALLOW_COPY_AND_ASSIGN(FullCardRequestTest);
};

// Matches the |arg| credit card to the given |record_type| and |card_number|.
MATCHER_P2(CardMatches, record_type, card_number, "") {
  return arg.record_type() == record_type &&
         arg.GetRawInfo(CREDIT_CARD_NUMBER) == base::ASCIIToUTF16(card_number);
}

// Matches the |arg| credit card to the given |record_type|, card |number|,
// expiration |month|, and expiration |year|.
MATCHER_P4(CardMatches, record_type, number, month, year, "") {
  return arg.record_type() == record_type &&
         arg.GetRawInfo(CREDIT_CARD_NUMBER) == base::ASCIIToUTF16(number) &&
         arg.GetRawInfo(CREDIT_CARD_EXP_MONTH) == base::ASCIIToUTF16(month) &&
         arg.GetRawInfo(CREDIT_CARD_EXP_4_DIGIT_YEAR) ==
             base::ASCIIToUTF16(year);
}

// Verify getting the full PAN and the CVC for a masked server card.
TEST_F(FullCardRequestTest, GetFullCardPanAndCvcForMaskedServerCard) {
  EXPECT_CALL(
      *delegate(),
      OnFullCardDetails(CardMatches(CreditCard::FULL_SERVER_CARD, "4111"),
                        base::ASCIIToUTF16("123")));
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*client(), OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  ui_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");
  ui_delegate()->OnUnmaskPromptClosed();
}

// Verify getting the CVC for a local card.
TEST_F(FullCardRequestTest, GetFullCardPanAndCvcForLocalCard) {
  EXPECT_CALL(*delegate(),
              OnFullCardDetails(CardMatches(CreditCard::LOCAL_CARD, "4111"),
                                base::ASCIIToUTF16("123")));
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*client(), OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  request()->GetFullCard(CreditCard(base::ASCIIToUTF16("4111"), 12, 2050),
                         AutofillClient::UNMASK_FOR_AUTOFILL,
                         delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  ui_delegate()->OnUnmaskResponse(response);
  ui_delegate()->OnUnmaskPromptClosed();
}

// Verify getting the CVC for an unmasked server card.
TEST_F(FullCardRequestTest, GetFullCardPanAndCvcForFullServerCard) {
  EXPECT_CALL(
      *delegate(),
      OnFullCardDetails(CardMatches(CreditCard::FULL_SERVER_CARD, "4111"),
                        base::ASCIIToUTF16("123")));
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*client(), OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  CreditCard full_server_card(base::ASCIIToUTF16("4111"), 12, 2050);
  full_server_card.set_record_type(CreditCard::FULL_SERVER_CARD);
  request()->GetFullCard(full_server_card, AutofillClient::UNMASK_FOR_AUTOFILL,
                         delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  ui_delegate()->OnUnmaskResponse(response);
  ui_delegate()->OnUnmaskPromptClosed();
}

// Only one request at a time should be allowed.
TEST_F(FullCardRequestTest, OneRequestAtATime) {
  EXPECT_CALL(*delegate(), OnFullCardError());
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*client(), OnUnmaskVerificationResult(_)).Times(0);

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id_1"),
      AutofillClient::UNMASK_FOR_AUTOFILL, delegate()->AsWeakPtr());
  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id_2"),
      AutofillClient::UNMASK_FOR_PAYMENT_REQUEST, delegate()->AsWeakPtr());
}

// After the first request completes, it's OK to start the second request.
TEST_F(FullCardRequestTest, SecondRequestOkAfterFirstFinished) {
  EXPECT_CALL(*delegate(), OnFullCardError()).Times(0);
  EXPECT_CALL(*delegate(),
              OnFullCardDetails(CardMatches(CreditCard::LOCAL_CARD, "4111"),
                                base::ASCIIToUTF16("123")))
      .Times(2);
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _)).Times(2);
  EXPECT_CALL(*client(), OnUnmaskVerificationResult(AutofillClient::SUCCESS))
      .Times(2);

  request()->GetFullCard(CreditCard(base::ASCIIToUTF16("4111"), 12, 2050),
                         AutofillClient::UNMASK_FOR_AUTOFILL,
                         delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  ui_delegate()->OnUnmaskResponse(response);
  ui_delegate()->OnUnmaskPromptClosed();

  request()->GetFullCard(CreditCard(base::ASCIIToUTF16("4111"), 12, 2050),
                         AutofillClient::UNMASK_FOR_AUTOFILL,
                         delegate()->AsWeakPtr());
  ui_delegate()->OnUnmaskResponse(response);
  ui_delegate()->OnUnmaskPromptClosed();
}

// If the user cancels the CVC prompt,
// FullCardRequest::Delegate::OnFullCardError() should be invoked.
TEST_F(FullCardRequestTest, ClosePromptWithoutUserInput) {
  EXPECT_CALL(*delegate(), OnFullCardError());
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*client(), OnUnmaskVerificationResult(_)).Times(0);

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, delegate()->AsWeakPtr());
  ui_delegate()->OnUnmaskPromptClosed();
}

// If the server provides an empty PAN,
// FullCardRequest::Delegate::OnFullCardError() should be invoked.
TEST_F(FullCardRequestTest, EmptyFullPan) {
  EXPECT_CALL(*delegate(), OnFullCardError());
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*client(),
              OnUnmaskVerificationResult(AutofillClient::PERMANENT_FAILURE));

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  ui_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::PERMANENT_FAILURE, "");
  ui_delegate()->OnUnmaskPromptClosed();
}

// Verify updating expiration date for a masked server card.
TEST_F(FullCardRequestTest, UpdateExpDateForMaskedServerCard) {
  EXPECT_CALL(*delegate(),
              OnFullCardDetails(CardMatches(CreditCard::FULL_SERVER_CARD,
                                            "4111", "12", "2050"),
                                base::ASCIIToUTF16("123")));
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*client(), OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  response.exp_month = base::ASCIIToUTF16("12");
  response.exp_year = base::ASCIIToUTF16("2050");
  ui_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");
  ui_delegate()->OnUnmaskPromptClosed();
}

// Verify updating expiration date for an unmasked server card.
TEST_F(FullCardRequestTest, UpdateExpDateForFullServerCard) {
  EXPECT_CALL(*delegate(),
              OnFullCardDetails(CardMatches(CreditCard::FULL_SERVER_CARD,
                                            "4111", "12", "2050"),
                                base::ASCIIToUTF16("123")));
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*client(), OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  CreditCard full_server_card(base::ASCIIToUTF16("4111"), 10, 2000);
  full_server_card.set_record_type(CreditCard::FULL_SERVER_CARD);
  request()->GetFullCard(full_server_card, AutofillClient::UNMASK_FOR_AUTOFILL,
                         delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  response.exp_month = base::ASCIIToUTF16("12");
  response.exp_year = base::ASCIIToUTF16("2050");
  ui_delegate()->OnUnmaskResponse(response);
  ui_delegate()->OnUnmaskPromptClosed();
}

// Verify updating expiration date for a local card.
TEST_F(FullCardRequestTest, UpdateExpDateForLocalCard) {
  EXPECT_CALL(*delegate(), OnFullCardDetails(CardMatches(CreditCard::LOCAL_CARD,
                                                         "4111", "12", "2050"),
                                             base::ASCIIToUTF16("123")));
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*client(), OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  request()->GetFullCard(CreditCard(base::ASCIIToUTF16("4111"), 10, 2000),
                         AutofillClient::UNMASK_FOR_AUTOFILL,
                         delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  response.exp_month = base::ASCIIToUTF16("12");
  response.exp_year = base::ASCIIToUTF16("2050");
  ui_delegate()->OnUnmaskResponse(response);
  ui_delegate()->OnUnmaskPromptClosed();
}

// Verify saving full PAN on disk.
TEST_F(FullCardRequestTest, SaveRealPan) {
  EXPECT_CALL(*delegate(),
              OnFullCardDetails(CardMatches(CreditCard::FULL_SERVER_CARD,
                                            "4111", "12", "2050"),
                                base::ASCIIToUTF16("123")));
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*client(), OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  response.exp_month = base::ASCIIToUTF16("12");
  response.exp_year = base::ASCIIToUTF16("2050");
  response.should_store_pan = true;
  ui_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");
  ui_delegate()->OnUnmaskPromptClosed();
}

// Verify getting full PAN and CVC for PaymentRequest.
TEST_F(FullCardRequestTest, UnmaskForPaymentRequest) {
  EXPECT_CALL(
      *delegate(),
      OnFullCardDetails(CardMatches(CreditCard::FULL_SERVER_CARD, "4111"),
                        base::ASCIIToUTF16("123")));
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*client(), OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_PAYMENT_REQUEST, delegate()->AsWeakPtr());
  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  ui_delegate()->OnUnmaskResponse(response);
  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");
  ui_delegate()->OnUnmaskPromptClosed();
}

// Verify that FullCardRequest::IsGettingFullCard() is true until the server
// returns the full PAN for a masked card.
TEST_F(FullCardRequestTest, IsGettingFullCardForMaskedServerCard) {
  EXPECT_CALL(
      *delegate(),
      OnFullCardDetails(CardMatches(CreditCard::FULL_SERVER_CARD, "4111"),
                        base::ASCIIToUTF16("123")));
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*client(), OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  EXPECT_FALSE(request()->IsGettingFullCard());

  request()->GetFullCard(
      CreditCard(CreditCard::MASKED_SERVER_CARD, "server_id"),
      AutofillClient::UNMASK_FOR_AUTOFILL, delegate()->AsWeakPtr());

  EXPECT_TRUE(request()->IsGettingFullCard());

  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  ui_delegate()->OnUnmaskResponse(response);

  EXPECT_TRUE(request()->IsGettingFullCard());

  OnDidGetRealPan(AutofillClient::SUCCESS, "4111");

  EXPECT_FALSE(request()->IsGettingFullCard());

  ui_delegate()->OnUnmaskPromptClosed();

  EXPECT_FALSE(request()->IsGettingFullCard());
}

// Verify that FullCardRequest::IsGettingFullCard() is true until the user types
// in the CVC for a card that is not masked.
TEST_F(FullCardRequestTest, IsGettingFullCardForLocalCard) {
  EXPECT_CALL(*delegate(),
              OnFullCardDetails(CardMatches(CreditCard::LOCAL_CARD, "4111"),
                                base::ASCIIToUTF16("123")));
  EXPECT_CALL(*client(), ShowUnmaskPrompt(_, _, _));
  EXPECT_CALL(*client(), OnUnmaskVerificationResult(AutofillClient::SUCCESS));

  EXPECT_FALSE(request()->IsGettingFullCard());

  request()->GetFullCard(CreditCard(base::ASCIIToUTF16("4111"), 12, 2050),
                         AutofillClient::UNMASK_FOR_AUTOFILL,
                         delegate()->AsWeakPtr());

  EXPECT_TRUE(request()->IsGettingFullCard());

  CardUnmaskDelegate::UnmaskResponse response;
  response.cvc = base::ASCIIToUTF16("123");
  ui_delegate()->OnUnmaskResponse(response);

  EXPECT_FALSE(request()->IsGettingFullCard());

  ui_delegate()->OnUnmaskPromptClosed();

  EXPECT_FALSE(request()->IsGettingFullCard());
}

}  // namespace payments
}  // namespace autofill
