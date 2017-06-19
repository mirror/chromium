// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CORE_AUTOFILL_PAYMENT_INSTRUMENT_DELEGATE_H_
#define COMPONENTS_PAYMENTS_CORE_AUTOFILL_PAYMENT_INSTRUMENT_DELEGATE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "components/autofill/core/browser/payments/full_card_request.h"

namespace autofill {
class CreditCard;
class PersonalDataManager;
}  // namespace autofill

namespace payments {

class AddressNormalizer;

class AutofillPaymentInstrumentDelegate {
 public:
  virtual ~AutofillPaymentInstrumentDelegate() {}

  // Gets the PersonalDataManager associated with this PaymentRequest flow.
  // Cannot be null.
  virtual autofill::PersonalDataManager* GetPersonalDataManager() const = 0;

  // Starts a FullCardRequest to unmask |credit_card|.
  virtual void DoFullCardRequest(
      const autofill::CreditCard& credit_card,
      base::WeakPtr<autofill::payments::FullCardRequest::ResultDelegate>
          result_delegate) = 0;

  // Returns a pointer to the address normalizer to use for the duration of this
  // Payment Request.
  virtual AddressNormalizer* GetAddressNormalizer() = 0;
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CORE_AUTOFILL_PAYMENT_INSTRUMENT_DELEGATE_H_
