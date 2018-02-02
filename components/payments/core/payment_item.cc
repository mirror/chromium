// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/payment_item.h"

#include "base/values.h"
#include "components/payments/core/payment_currency_amount.h"

namespace payments {

namespace {

// These are defined as part of the spec at:
// https://w3c.github.io/payment-request/#dom-paymentitem
static const char kPaymentItemAmount[] = "amount";
static const char kPaymentItemLabel[] = "label";
static const char kPaymentItemPending[] = "pending";

}  // namespace

std::unique_ptr<base::DictionaryValue> PaymentItemToDictionaryValue(
    const mojom::PaymentItem& item) {
  auto result = std::make_unique<base::DictionaryValue>();
  result->SetString(kPaymentItemLabel, item.label);
  result->SetDictionary(kPaymentItemAmount,
                        PaymentCurrencyAmountToDictionaryValue(*item.amount));
  result->SetBoolean(kPaymentItemPending, item.pending);

  return result;
}

bool PaymentItemFromDictionaryValue(const base::DictionaryValue& value,
                                    mojom::PaymentItem* item) {
  if (!value.GetString(kPaymentItemLabel, &item->label)) {
    return false;
  }

  const base::DictionaryValue* amount_dict = nullptr;
  if (!value.GetDictionary(kPaymentItemAmount, &amount_dict)) {
    return false;
  }
  item->amount = mojom::PaymentCurrencyAmount::New();
  if (!PaymentCurrencyAmountFromDictionaryValue(*amount_dict,
                                                item->amount.get())) {
    return false;
  }

  // Pending is optional.
  value.GetBoolean(kPaymentItemPending, &item->pending);

  return true;
}

}  // namespace payments
