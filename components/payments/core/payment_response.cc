// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/payment_response.h"

#include "base/memory/ptr_util.h"
#include "base/values.h"
#include "components/payments/core/payment_address.h"
#include "components/payments/core/util/json_util.h"

namespace payments {

namespace {

// These are defined as part of the spec at:
// https://www.w3.org/TR/payment-request/#paymentresponse-interface
static const char kPaymentResponseId[] = "requestId";
static const char kPaymentResponseDetails[] = "details";
static const char kPaymentResponseMethodName[] = "methodName";
static const char kPaymentResponsePayerEmail[] = "payerEmail";
static const char kPaymentResponsePayerName[] = "payerName";
static const char kPaymentResponsePayerPhone[] = "payerPhone";
static const char kPaymentResponseShippingAddress[] = "shippingAddress";
static const char kPaymentResponseShippingOption[] = "shippingOption";

}  // namespace

PaymentResponse::PaymentResponse() {}
PaymentResponse::~PaymentResponse() = default;

bool PaymentResponse::operator==(const PaymentResponse& other) const {
  return this->payment_request_id == other.payment_request_id &&
         this->method_name == other.method_name &&
         this->details == other.details &&
         ((!this->shipping_address && !other.shipping_address) ||
          (this->shipping_address && other.shipping_address &&
           *this->shipping_address == *other.shipping_address)) &&
         this->shipping_option == other.shipping_option &&
         this->payer_name == other.payer_name &&
         this->payer_email == other.payer_email &&
         this->payer_phone == other.payer_phone;
}

bool PaymentResponse::operator!=(const PaymentResponse& other) const {
  return !(*this == other);
}

std::unique_ptr<base::DictionaryValue> PaymentResponse::ToDictionaryValue()
    const {
  std::unique_ptr<base::DictionaryValue> result(new base::DictionaryValue());

  result->SetString(kPaymentResponseId, this->payment_request_id);
  result->SetString(kPaymentResponseMethodName, this->method_name);
  // |this.details| is a json-serialized string. Parse it to a base::Value so
  // that when |result| is converted to a JSON string, the "details" property
  // won't get json-escaped.
  std::unique_ptr<base::Value> details = ReadJSONStringToValue(this->details);
  result->Set(kPaymentResponseDetails,
              details ? std::move(details) : base::MakeUnique<base::Value>());
  result->Set(kPaymentResponseShippingAddress,
              this->shipping_address
                  ? this->shipping_address->ToDictionaryValue()
                  : base::MakeUnique<base::Value>());
  result->SetString(kPaymentResponseShippingOption, this->shipping_option);
  result->SetString(kPaymentResponsePayerName, this->payer_name);
  result->SetString(kPaymentResponsePayerEmail, this->payer_email);
  result->SetString(kPaymentResponsePayerPhone, this->payer_phone);
  return result;
}

}  // namespace payments
