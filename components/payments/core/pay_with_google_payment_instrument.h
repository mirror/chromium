// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CORE_PAY_WITH_GOOGLE_PAYMENT_INSTRUMENT_H_
#define COMPONENTS_PAYMENTS_CORE_PAY_WITH_GOOGLE_PAYMENT_INSTRUMENT_H_

#include "components/payments/core/payment_instrument.h"

namespace payments {

class PayWithGooglePaymentInstrument : public PaymentInstrument {
 public:
  PayWithGooglePaymentInstrument();
  ~PayWithGooglePaymentInstrument() override;

  void InvokePaymentApp(Delegate* delegate) override;
  bool IsCompleteForPayment() const override;
  bool IsExactlyMatchingMerchantRequest() const override;
  base::string16 GetMissingInfoLabel() const override;
  bool IsValidForCanMakePayment() const override;
  void RecordUse() override;
  base::string16 GetLabel() const override;
  base::string16 GetSublabel() const override;
  bool IsValidForModifier(const std::vector<std::string>& methods,
                          bool supported_networks_specified,
                          const std::set<std::string>& supported_networks,
                          bool supported_types_specified,
                          const std::set<autofill::CreditCard::CardType>&
                              supported_types) const override;
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CORE_PAY_WITH_GOOGLE_PAYMENT_INSTRUMENT_H_
