// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/service_worker_payment_instrument.h"

namespace payments {
ServiceWorkerPaymentInstrument::ServiceWorkerPaymentInstrument(
    std::unique_ptr<StoredPaymentApp> stored_payment_app_info)
    : stored_payment_app_info_(std::move(stored_payment_app_info)),
      PaymentInstrument("", 0, PaymentInstrument::Type::SERVICE_WORKER_APP) {}

ServiceWorkerPaymentInstrument::~ServiceWorkerPaymentInstrument() {}

void ServiceWorkerPaymentInstrument::InvokePaymentApp(
    PaymentInstrument::Delegate* delegate) {
  NOTIMPLEMENTED();
}

bool ServiceWorkerPaymentInstrument::IsCompleteForPayment() const {
  return true;
}

bool ServiceWorkerPaymentInstrument::IsExactlyMatchingMerchantRequest() const {
  return true;
}

base::string16 ServiceWorkerPaymentInstrument::GetMissingInfoLabel() const {
  NOTREACHED();
  return "";
}

bool ServiceWorkerPaymentInstrument::IsValidForCanMakePayment() const {
  NOTIMPLEMENTED();
}

void RecordUse() {
  NOTIMPLEMENTED();
}

base::string16 ServiceWorkerPaymentInstrument::GetLabel() const {
  return base::UTF8ToUTF16(stored_payment_app_info_->name);
}

base::string16 ServiceWorkerPaymentInstrument::GetSublabel() const {
  return base::UTF8ToUTF16(stored_payment_app_info_->scope.GetOrigin().spec());
}

bool ServiceWorkerPaymentInstrument::IsValidForModifier(
    const std::vector<std::string>& method,
    const std::vector<std::string>& supported_networks,
    const std::set<autofill::CreditCard::CardType>& supported_types,
    bool supported_types_specified) const {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace payments
