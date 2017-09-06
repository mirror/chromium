// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CORE_SERVICE_WORKER_PAYMENT_INSTRUMENT_H_
#define COMPONENTS_PAYMENTS_CORE_SERVICE_WORKER_PAYMENT_INSTRUMENT_H_

#include "components/payments/core/payment_instrument.h"

namespace payments {

class ServiceWorkerPaymentInstrument : public PaymentInstrument {
 public:
  ServiceWorkerPaymentInstrument(
      std::unique_ptr<StoredPaymentApp> stored_payment_app_info);
  ~ServiceWorkerPaymentInstrument() override;

  // PaymentInstrument:
  void InvokePaymentApp(PaymentInstrument::Delegate* delegate) override;
  bool IsCompleteForPayment() const override;
  bool IsExactlyMatchingMerchantRequest() const override;
  base::string16 GetMissingInfoLabel() const override;
  bool IsValidForCanMakePayment() const override;
  void RecordUse() override;
  base::string16 GetLabel() const override;
  base::string16 GetSublabel() const override;
  bool IsValidForModifier(
      const std::vector<std::string>& method,
      const std::vector<std::string>& supported_networks,
      const std::set<autofill::CreditCard::CardType>& supported_types,
      bool supported_types_specified) const override;

 private:
  std::unique_ptr<StoredPaymentApp> stored_payment_app_info_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerPaymentInstrument);
};
}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CORE_SERVICE_WORKER_PAYMENT_INSTRUMENT_H_
