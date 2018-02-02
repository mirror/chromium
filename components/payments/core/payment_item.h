// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CORE_PAYMENT_ITEM_H_
#define COMPONENTS_PAYMENTS_CORE_PAYMENT_ITEM_H_

#include <memory>
#include <string>

#include "components/payments/mojom/payment_request_data.mojom.h"

namespace base {
class DictionaryValue;
}

namespace payments {

bool PaymentItemFromDictionaryValue(const base::DictionaryValue& value,
                                    mojom::PaymentItem* item);

std::unique_ptr<base::DictionaryValue> PaymentItemToDictionaryValue(
    const mojom::PaymentItem& item);

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CORE_PAYMENT_ITEM_H_
