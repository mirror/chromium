// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/service_worker_payment_instrument.h"

#include "components/payments/content/payment_request_spec.h"
#include "content/public/browser/stored_payment_app.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/WebKit/public/platform/modules/payments/payment_request.mojom.h"

namespace payments {

class ServiceWorkerPaymentInstrumentTest : public testing::Test,
                                           public PaymentRequestSpec::Observer {
 protected:
  ServiceWorkerPaymentInstrumentTest() {}
  ~ServiceWorkerPaymentInstrumentTest() override {}

  void SetUp() override;

  void OnSpecUpdated() override;

  ServiceWorkerPaymentInstrument* instrument() {
    return service_worker_instrument_.get();
  }

 private:
  std::unique_ptr<ServiceWorkerPaymentInstrument> service_worker_instrument_;
  std::unique_ptr<PaymentRequestSpec> spec_;
};

void ServiceWorkerPaymentInstrumentTest::SetUp() {
  testing::Test::SetUp();

  // Create PaymentMethodData.
  std::vector<mojom::PaymentMethodDataPtr> method_data;
  mojom::PaymentMethodDataPtr method_1 = mojom::PaymentMethodData::New();
  method_1->supported_methods.push_back("basic-card");
  method_1->supported_networks.push_back(mojom::BasicCardNetwork::VISA);
  method_data.push_back(std::move(method_1));
  mojom::PaymentMethodDataPtr method_2 = mojom::PaymentMethodData::New();
  method_2->supported_methods.push_back("https://bobpay.com");
  method_data.push_back(std::move(method_2));

  // Create PaymentDetails.
  mojom::PaymentDetailsPtr details = mojom::PaymentDetails::New();
  details->total = mojom::PaymentItem::New();
  details->total->label = "donation";
  details->total->amount = mojom::PaymentCurrencyAmount::New();
  details->total->amount->currency = "USD";
  details->total->amount->value = "50";

  mojom::PaymentItemPtr display_item = mojom::PaymentItem::New();
  display_item->label = "family discount";
  display_item->amount = mojom::PaymentCurrencyAmount::New();
  display_item->amount->currency = "USD";
  display_item->amount->value = "-10.00";
  details->display_items.emplace_back(std::move(display_item));

  mojom::PaymentShippingOptionPtr option = mojom::PaymentShippingOption::New();
  option->id = "option:1";
  option->selected = true;
  details->shipping_options.emplace_back(std::move(option));

  mojom::PaymentDetailsModifierPtr modifier =
      mojom::PaymentDetailsModifier::New();
  modifier->total = mojom::PaymentItem::New();
  modifier->total->label = "basic-card discount";
  modifier->total->amount = mojom::PaymentCurrencyAmount::New();
  modifier->total->amount->currency = "USD";
  modifier->total->amount->value = "30";
  modifier->method_data = mojom::PaymentMethodData::New();
  modifier->method_data->supported_methods.push_back("basic-card");
  modifier->method_data->supported_networks.push_back(
      mojom::BasicCardNetwork::VISA);
  details->modifiers.emplace_back(std::move(modifier));

  details->id.value() = "test payment request";

  // Create PaymentOptions
  mojom::PaymentOptionsPtr options = mojom::PaymentOptions::New();
  options->request_payer_name = true;
  options->request_payer_email = false;
  options->request_payer_phone = true;
  options->request_shipping = true;

  // Create PaymentRequestSpec.
  spec_ = base::MakeUnique<PaymentRequestSpec>(
      std::move(options), std::move(details), std::move(method_data), this,
      "en-US");

  std::unique_ptr<content::StoredPaymentApp> app =
      base::MakeUnique<content::StoredPaymentApp>();
  app->scope = GURL("https://bobpay.com");
  app->name = "bobpay";
  app->enabled_methods.emplace_back("https://bobpay.com");
  app->enabled_methods.emplace_back("basic-card");
  app->user_hint = "test";

  service_worker_instrument_ = base::MakeUnique<ServiceWorkerPaymentInstrument>(
      nullptr, GURL("https://example.com"), GURL("https://example.com/pay"),
      spec_.get(), std::move(app));
}

void ServiceWorkerPaymentInstrumentTest::OnSpecUpdated() {}

TEST_F(ServiceWorkerPaymentInstrumentTest, IsValidForModifier) {
  std::vector<std::string> modifier_methods = {"basic-card"};
  std::vector<std::string> modifier_supported_netwokrs = {
      mojom::BasicCardNetwork::VISA};
  ASSERT_TRUE(instrument()->IsValidForModifier(
      std::vector<std::string>("basic-card"),
      std::vector<std::string>(mojom::BasicCardNetwork::VISA),
      std::set<autofill::CreditCard::CardType>(), false));
}
}  // namespace payments

}  // namespace payments
