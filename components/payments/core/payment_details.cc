// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/payment_details.h"

#include <algorithm>

#include "base/values.h"

namespace payments {

namespace {

// These are defined as part of the spec at:
// https://w3c.github.io/payment-request/#payment-details-dictionaries
static const char kPaymentDetailsAdditionalDisplayItems[] =
    "additionalDisplayItems";
static const char kPaymentDetailsDisplayItems[] = "displayItems";
static const char kPaymentDetailsError[] = "error";
static const char kPaymentDetailsId[] = "id";
static const char kPaymentDetailsModifiers[] = "modifiers";
static const char kPaymentDetailsShippingOptions[] = "shippingOptions";
static const char kPaymentDetailsTotal[] = "total";

}  // namespace

PaymentDetails::PaymentDetails() {}
PaymentDetails::~PaymentDetails() = default;

PaymentDetails::PaymentDetails(const PaymentDetails& other) {
  *this = other;
}

PaymentDetails& PaymentDetails::operator=(const PaymentDetails& other) {
  id = other.id;
  if (other.total)
    total = other.total.Clone();
  else
    total.reset();

  display_items.clear();
  display_items.reserve(other.display_items.size());
  for (const auto& item : other.display_items) {
    display_items.push_back(item.Clone());
  }

  shipping_options = std::vector<PaymentShippingOption>(other.shipping_options);
  modifiers = std::vector<PaymentDetailsModifier>(other.modifiers);
  return *this;
}

bool PaymentDetails::operator==(const PaymentDetails& other) const {
  return id == other.id && total.Equals(other.total) &&
         display_items == other.display_items &&
         shipping_options == other.shipping_options &&
         modifiers == other.modifiers && error == other.error;
}

bool PaymentDetails::operator!=(const PaymentDetails& other) const {
  return !(*this == other);
}

bool PaymentDetails::FromDictionaryValue(const base::DictionaryValue& value,
                                         bool requires_total) {
  display_items.clear();
  shipping_options.clear();
  modifiers.clear();

  // ID is optional.
  value.GetString(kPaymentDetailsId, &id);

  const base::DictionaryValue* total_dict = nullptr;
  if (!value.GetDictionary(kPaymentDetailsTotal, &total_dict) &&
      requires_total) {
    return false;
  }
  if (total_dict) {
    total = mojom::PaymentItem::New();
    if (!PaymentItemFromDictionaryValue(*total_dict, total.get()))
      return false;
  }

  const base::ListValue* display_items_list = nullptr;
  if (value.GetList(kPaymentDetailsDisplayItems, &display_items_list)) {
    for (size_t i = 0; i < display_items_list->GetSize(); ++i) {
      const base::DictionaryValue* payment_item_dict = nullptr;
      if (!display_items_list->GetDictionary(i, &payment_item_dict)) {
        return false;
      }
      auto payment_item = mojom::PaymentItem::New();
      if (!PaymentItemFromDictionaryValue(*payment_item_dict,
                                          payment_item.get())) {
        return false;
      }
      display_items.push_back(payment_item.Clone());
    }
  }

  const base::ListValue* shipping_options_list = nullptr;
  if (value.GetList(kPaymentDetailsShippingOptions, &shipping_options_list)) {
    for (size_t i = 0; i < shipping_options_list->GetSize(); ++i) {
      const base::DictionaryValue* shipping_option_dict = nullptr;
      if (!shipping_options_list->GetDictionary(i, &shipping_option_dict)) {
        return false;
      }
      PaymentShippingOption shipping_option;
      if (!shipping_option.FromDictionaryValue(*shipping_option_dict)) {
        return false;
      }
      shipping_options.push_back(shipping_option);
    }
  }

  const base::ListValue* modifiers_list = nullptr;
  if (value.GetList(kPaymentDetailsModifiers, &modifiers_list)) {
    for (size_t i = 0; i < modifiers_list->GetSize(); ++i) {
      PaymentDetailsModifier modifier;
      const base::DictionaryValue* modifier_dict = nullptr;
      if (!modifiers_list->GetDictionary(i, &modifier_dict) ||
          !modifier.method_data.FromDictionaryValue(*modifier_dict)) {
        return false;
      }
      const base::DictionaryValue* modifier_total_dict = nullptr;
      if (modifier_dict->GetDictionary(kPaymentDetailsTotal,
                                       &modifier_total_dict)) {
        modifier.total = mojom::PaymentItem::New();
        if (!PaymentItemFromDictionaryValue(*modifier_total_dict,
                                            modifier.total.get()))
          return false;
      }
      const base::ListValue* additional_display_items_list = nullptr;
      if (modifier_dict->GetList(kPaymentDetailsAdditionalDisplayItems,
                                 &additional_display_items_list)) {
        for (size_t j = 0; j < additional_display_items_list->GetSize(); ++j) {
          const base::DictionaryValue* additional_display_item_dict = nullptr;
          auto additional_display_item = mojom::PaymentItem::New();
          if (!additional_display_items_list->GetDictionary(
                  i, &additional_display_item_dict) ||
              PaymentItemFromDictionaryValue(*additional_display_item_dict,
                                             additional_display_item.get())) {
            return false;
          }
          modifier.additional_display_items.push_back(
              additional_display_item.Clone());
        }
      }
      modifiers.push_back(modifier);
    }
  }

  // Error is optional.
  value.GetString(kPaymentDetailsError, &error);

  return true;
}

}  // namespace payments
