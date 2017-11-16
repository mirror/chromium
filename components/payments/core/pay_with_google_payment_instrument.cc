// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/pay_with_google_payment_instrument.h"

#include "base/bind.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_data_util.h"
#include "components/payments/core/payment_request_base_delegate.h"

namespace payments {

PayWithGooglePaymentInstrument::PayWithGooglePaymentInstrument(
    PaymentRequestBaseDelegate* payment_request_delegate)
    : PaymentInstrument(
          autofill::data_util::GetPaymentRequestData("google").icon_resource_id,
          PaymentInstrument::Type::GOOGLE),
      payment_request_delegate_(payment_request_delegate) {}

PayWithGooglePaymentInstrument::~PayWithGooglePaymentInstrument() {}

void PayWithGooglePaymentInstrument::InvokePaymentApp(Delegate* delegate) {
  /*const std::string kPaymentMethod = "https://google.com/pay";
  const GURL kFlowOverrideURL("https://google.com/pay");
  const GURL kSuccessURL("https://google.com/pay/success");
  const GURL kFailureURL("https://google.com/pay/failure");*/

  const std::string kPaymentMethod =
      "https://anthonyvd.github.io/pr/handlers/custom-handler";
  const GURL kFlowOverrideURL(
      "https://anthonyvd.github.io/pr/handlers/custom-handler/handler/"
      "index.html");
  const GURL kSuccessURL(
      "https://anthonyvd.github.io/pr/handlers/custom-handler/handler/success/"
      "index.html");
  const GURL kFailureURL(
      "https://anthonyvd.github.io/pr/handlers/custom-handler/handler/failure/"
      "index.html");
  payment_request_delegate_->ShowWebPaymentHandlerFlow(
      kPaymentMethod, kFlowOverrideURL, kSuccessURL, kFailureURL,
      base::Bind(&PaymentInstrument::Delegate::OnInstrumentDetailsReady,
                 base::Unretained(delegate)));
}

bool PayWithGooglePaymentInstrument::IsCompleteForPayment() const {
  return true;
}

bool PayWithGooglePaymentInstrument::IsExactlyMatchingMerchantRequest() const {
  return true;
}

base::string16 PayWithGooglePaymentInstrument::GetMissingInfoLabel() const {
  NOTREACHED();
  return base::string16();
}

bool PayWithGooglePaymentInstrument::IsValidForCanMakePayment() const {
  return true;
}

void PayWithGooglePaymentInstrument::RecordUse() {}

base::string16 PayWithGooglePaymentInstrument::GetLabel() const {
  return base::ASCIIToUTF16("Pay with Google");
}

base::string16 PayWithGooglePaymentInstrument::GetSublabel() const {
  return base::ASCIIToUTF16("Pay with Google");
}

bool PayWithGooglePaymentInstrument::IsValidForModifier(
    const std::vector<std::string>& methods,
    bool supported_networks_specified,
    const std::set<std::string>& supported_networks,
    bool supported_types_specified,
    const std::set<autofill::CreditCard::CardType>& supported_types) const {
  // TODO(anthonyvd): wire this up properly.
  return true;
}

}  // namespace payments
