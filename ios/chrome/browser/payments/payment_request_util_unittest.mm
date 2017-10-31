// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/payments/payment_request_util.h"

#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "components/autofill/core/browser/credit_card.h"
#include "components/autofill/core/browser/field_types.h"
#include "components/payments/core/basic_card_response.h"
#include "components/payments/core/payment_address.h"
#include "components/payments/core/payment_response.h"
#include "ios/chrome/browser/payments/payment_request_unittest_base.h"
#import "ios/chrome/browser/payments/test_payment_request.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace payment_request_util {

class PaymentRequestUtilTest : public PaymentRequestUnitTestBase,
                               public PlatformTest {
 protected:
  PaymentRequestUtilTest() {}

  void SetUp() override { PaymentRequestUnitTestBase::SetUp(); }
};

// Tests that payment_request_util::RequestContactInfo returns true if payer's
// name, phone number, or email address are requested and false otherwise.
TEST_F(PaymentRequestUtilTest, RequestContactInfo) {
  payments::WebPaymentRequest web_payment_request;

  web_payment_request.options.request_payer_name = true;
  web_payment_request.options.request_payer_phone = true;
  web_payment_request.options.request_payer_email = true;
  payments::TestPaymentRequest payment_request1(web_payment_request,
                                                browser_state(), web_state(),
                                                personal_data_manager());
  EXPECT_TRUE(RequestContactInfo(payment_request1));

  web_payment_request.options.request_payer_name = false;
  payments::TestPaymentRequest payment_request2(web_payment_request,
                                                browser_state(), web_state(),
                                                personal_data_manager());
  EXPECT_TRUE(RequestContactInfo(payment_request2));

  web_payment_request.options.request_payer_phone = false;
  payments::TestPaymentRequest payment_request3(web_payment_request,
                                                browser_state(), web_state(),
                                                personal_data_manager());
  EXPECT_TRUE(RequestContactInfo(payment_request3));

  web_payment_request.options.request_payer_email = false;
  payments::TestPaymentRequest payment_request4(web_payment_request,
                                                browser_state(), web_state(),
                                                personal_data_manager());
  EXPECT_FALSE(RequestContactInfo(payment_request4));
}

// Tests that payment_request_util::RequestShipping returns true if
// requestShipping of the PaymentOptions is true.
TEST_F(PaymentRequestUtilTest, RequestShipping) {
  payments::WebPaymentRequest web_payment_request;

  web_payment_request.options.request_shipping = true;
  payments::TestPaymentRequest payment_request1(web_payment_request,
                                                browser_state(), web_state(),
                                                personal_data_manager());
  EXPECT_TRUE(RequestShipping(payment_request1));

  web_payment_request.options.request_shipping = false;
  payments::TestPaymentRequest payment_request2(web_payment_request,
                                                browser_state(), web_state(),
                                                personal_data_manager());
  EXPECT_FALSE(RequestShipping(payment_request2));
}

// Tests the output of payment_request_util::CanPay.
TEST_F(PaymentRequestUtilTest, CanPay) {
  payments::WebPaymentRequest web_payment_request;
  payments::PaymentMethodData method_datum;
  method_datum.supported_methods.push_back("basic-card");
  method_datum.supported_networks.push_back("visa");
  web_payment_request.method_data.push_back(method_datum);

  // No selected payment method.
  payments::TestPaymentRequest payment_request1(web_payment_request,
                                                browser_state(), web_state(),
                                                personal_data_manager());
  EXPECT_FALSE(CanPay(payment_request1));

  autofill::AutofillProfile profile = autofill::test::GetFullProfile();
  // Make the profile incomplete by removing the name and the phone number so
  // that it is not selected as the default shipping address or contact info.
  profile.SetInfo(autofill::AutofillType(autofill::NAME_FULL), base::string16(),
                  "en-US");
  profile.SetInfo(autofill::AutofillType(autofill::PHONE_HOME_WHOLE_NUMBER),
                  base::string16(), "en-US");
  autofill::CreditCard card = autofill::test::GetCreditCard();  // Visa.
  card.set_billing_address_id(profile.guid());
  AddAutofillProfile(std::move(profile));
  AddCreditCard(std::move(card));

  // Has a selected payment method.
  payments::TestPaymentRequest payment_request2(web_payment_request,
                                                browser_state(), web_state(),
                                                personal_data_manager());
  EXPECT_TRUE(CanPay(payment_request2));

  // No selected contact info.
  web_payment_request.options.request_payer_phone = true;
  payments::TestPaymentRequest payment_request3(web_payment_request,
                                                browser_state(), web_state(),
                                                personal_data_manager());
  EXPECT_FALSE(CanPay(payment_request3));

  profiles()[0]->SetInfo(
      autofill::AutofillType(autofill::PHONE_HOME_WHOLE_NUMBER),
      base::ASCIIToUTF16("16502111111"), "en-US");

  // Has a selected contact info.
  payments::TestPaymentRequest payment_request4(web_payment_request,
                                                browser_state(), web_state(),
                                                personal_data_manager());
  EXPECT_TRUE(CanPay(payment_request4));

  // No selected shipping address.
  web_payment_request.options.request_shipping = true;
  payments::TestPaymentRequest payment_request5(web_payment_request,
                                                browser_state(), web_state(),
                                                personal_data_manager());
  EXPECT_FALSE(CanPay(payment_request5));

  profiles()[0]->SetInfo(autofill::AutofillType(autofill::NAME_FULL),
                         base::ASCIIToUTF16("John Doe"), "en-US");

  // Has a selected shipping address, but no selected shipping option.
  payments::TestPaymentRequest payment_request6(web_payment_request,
                                                browser_state(), web_state(),
                                                personal_data_manager());
  EXPECT_FALSE(CanPay(payment_request6));

  std::vector<payments::PaymentShippingOption> shipping_options;
  payments::PaymentShippingOption option;
  option.id = "1";
  option.selected = true;
  shipping_options.push_back(std::move(option));
  web_payment_request.details.shipping_options = std::move(shipping_options);

  // Has a selected shipping address and a selected shipping option.
  payments::TestPaymentRequest payment_request7(web_payment_request,
                                                browser_state(), web_state(),
                                                personal_data_manager());
  EXPECT_TRUE(CanPay(payment_request7));
}

// Tests that serializing a default PaymentResponse yields the expected result.
TEST_F(PaymentRequestUtilTest,
       PaymentResponseToDictionaryValue_EmptyResponseDictionary) {
  base::DictionaryValue expected_value;

  expected_value.SetString("requestId", "");
  expected_value.SetString("methodName", "");
  expected_value.Set("details", std::make_unique<base::Value>());
  expected_value.Set("shippingAddress", std::make_unique<base::Value>());
  expected_value.SetString("shippingOption", "");
  expected_value.SetString("payerName", "");
  expected_value.SetString("payerEmail", "");
  expected_value.SetString("payerPhone", "");

  payments::PaymentResponse payment_response;
  EXPECT_TRUE(expected_value.Equals(
      PaymentResponseToDictionaryValue(payment_response).get()));
}

// Tests that serializing a populated PaymentResponse yields the expected
// result.
TEST_F(PaymentRequestUtilTest,
       PaymentResponseToDictionaryValue_PopulatedResponseDictionary) {
  base::DictionaryValue expected_value;

  auto details = std::make_unique<base::DictionaryValue>();
  details->SetString("cardNumber", "1111-1111-1111-1111");
  details->SetString("cardholderName", "Jon Doe");
  details->SetString("expiryMonth", "02");
  details->SetString("expiryYear", "2090");
  details->SetString("cardSecurityCode", "111");
  auto billing_address = std::make_unique<base::DictionaryValue>();
  billing_address->SetString("country", "");
  billing_address->Set("addressLine", std::make_unique<base::ListValue>());
  billing_address->SetString("region", "");
  billing_address->SetString("dependentLocality", "");
  billing_address->SetString("city", "");
  billing_address->SetString("postalCode", "90210");
  billing_address->SetString("languageCode", "");
  billing_address->SetString("sortingCode", "");
  billing_address->SetString("organization", "");
  billing_address->SetString("recipient", "");
  billing_address->SetString("phone", "");
  details->Set("billingAddress", std::move(billing_address));
  expected_value.Set("details", std::move(details));
  expected_value.SetString("requestId", "12345");
  expected_value.SetString("methodName", "American Express");
  auto shipping_address = std::make_unique<base::DictionaryValue>();
  shipping_address->SetString("country", "");
  shipping_address->Set("addressLine", std::make_unique<base::ListValue>());
  shipping_address->SetString("region", "");
  shipping_address->SetString("dependentLocality", "");
  shipping_address->SetString("city", "");
  shipping_address->SetString("postalCode", "94115");
  shipping_address->SetString("languageCode", "");
  shipping_address->SetString("sortingCode", "");
  shipping_address->SetString("organization", "");
  shipping_address->SetString("recipient", "");
  shipping_address->SetString("phone", "");
  expected_value.Set("shippingAddress", std::move(shipping_address));
  expected_value.SetString("shippingOption", "666");
  expected_value.SetString("payerName", "Jane Doe");
  expected_value.SetString("payerEmail", "jane@example.com");
  expected_value.SetString("payerPhone", "1234-567-890");

  payments::PaymentResponse payment_response;
  payment_response.payment_request_id = "12345";
  payment_response.method_name = "American Express";

  payments::BasicCardResponse payment_response_details;
  payment_response_details.card_number =
      base::ASCIIToUTF16("1111-1111-1111-1111");
  payment_response_details.cardholder_name = base::ASCIIToUTF16("Jon Doe");
  payment_response_details.expiry_month = base::ASCIIToUTF16("02");
  payment_response_details.expiry_year = base::ASCIIToUTF16("2090");
  payment_response_details.card_security_code = base::ASCIIToUTF16("111");
  payment_response_details.billing_address.postal_code =
      base::ASCIIToUTF16("90210");
  std::unique_ptr<base::DictionaryValue> response_value =
      payment_response_details.ToDictionaryValue();
  std::string payment_response_stringified_details;
  base::JSONWriter::Write(*response_value,
                          &payment_response_stringified_details);
  payment_response.details = payment_response_stringified_details;

  payment_response.shipping_address =
      std::make_unique<payments::PaymentAddress>();
  payment_response.shipping_address->postal_code = base::ASCIIToUTF16("94115");
  payment_response.shipping_option = "666";
  payment_response.payer_name = base::ASCIIToUTF16("Jane Doe");
  payment_response.payer_email = base::ASCIIToUTF16("jane@example.com");
  payment_response.payer_phone = base::ASCIIToUTF16("1234-567-890");
  EXPECT_TRUE(expected_value.Equals(
      PaymentResponseToDictionaryValue(payment_response).get()));
}

}  // namespace payment_request_util
