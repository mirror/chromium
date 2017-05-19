// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/strings_util.h"

#include "base/logging.h"
#include "components/strings/grit/components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace payments {

base::string16 GetShippingAddressSelectorInfoMessage(
    PaymentShippingType shipping_type) {
  switch (shipping_type) {
    case payments::PaymentShippingType::DELIVERY:
      return l10n_util::GetStringUTF16(
          IDS_PAYMENTS_SELECT_DELIVERY_ADDRESS_FOR_DELIVERY_METHODS);
    case payments::PaymentShippingType::PICKUP:
      return l10n_util::GetStringUTF16(
          IDS_PAYMENTS_SELECT_PICKUP_ADDRESS_FOR_PICKUP_METHODS);
    case payments::PaymentShippingType::SHIPPING:
      return l10n_util::GetStringUTF16(
          IDS_PAYMENTS_SELECT_SHIPPING_ADDRESS_FOR_SHIPPING_METHODS);
    default:
      NOTREACHED();
      return base::string16();
  }
}

base::string16 GetShippingAddressSectionString(
    PaymentShippingType shipping_type) {
  switch (shipping_type) {
    case PaymentShippingType::DELIVERY:
      return l10n_util::GetStringUTF16(IDS_PAYMENTS_DELIVERY_ADDRESS_LABEL);
    case PaymentShippingType::PICKUP:
      return l10n_util::GetStringUTF16(IDS_PAYMENTS_PICKUP_ADDRESS_LABEL);
    case PaymentShippingType::SHIPPING:
      return l10n_util::GetStringUTF16(IDS_PAYMENTS_SHIPPING_ADDRESS_LABEL);
    default:
      NOTREACHED();
      return base::string16();
  }
}

base::string16 GetShippingOptionSectionString(
    PaymentShippingType shipping_type) {
  switch (shipping_type) {
    case PaymentShippingType::DELIVERY:
      return l10n_util::GetStringUTF16(IDS_PAYMENTS_DELIVERY_OPTION_LABEL);
    case PaymentShippingType::PICKUP:
      return l10n_util::GetStringUTF16(IDS_PAYMENTS_PICKUP_OPTION_LABEL);
    case PaymentShippingType::SHIPPING:
      return l10n_util::GetStringUTF16(IDS_PAYMENTS_SHIPPING_OPTION_LABEL);
    default:
      NOTREACHED();
      return base::string16();
  }
}

base::string16 GetAcceptedCardTypesText(
    const std::set<autofill::CreditCard::CardType>& types) {
  int credit =
      types.find(autofill::CreditCard::CARD_TYPE_CREDIT) != types.end() ? 1 : 0;
  int debit =
      types.find(autofill::CreditCard::CARD_TYPE_DEBIT) != types.end() ? 1 : 0;
  int prepaid =
      types.find(autofill::CreditCard::CARD_TYPE_PREPAID) != types.end() ? 1
                                                                         : 0;
  int string_ids[2][2][2];

  // [credit][debit][prepaid]
  string_ids[0][0][0] = IDS_PAYMENTS_ACCEPTED_CARDS_LABEL;
  string_ids[0][0][1] = IDS_PAYMENTS_ACCEPTED_PREPAID_CARDS_LABEL;
  string_ids[0][1][0] = IDS_PAYMENTS_ACCEPTED_DEBIT_CARDS_LABEL;
  string_ids[0][1][1] = IDS_PAYMENTS_ACCEPTED_DEBIT_PREPAID_CARDS_LABEL;
  string_ids[1][0][0] = IDS_PAYMENTS_ACCEPTED_CREDIT_CARDS_LABEL;
  string_ids[1][0][1] = IDS_PAYMENTS_ACCEPTED_CREDIT_PREPAID_CARDS_LABEL;
  string_ids[1][1][0] = IDS_PAYMENTS_ACCEPTED_CREDIT_DEBIT_CARDS_LABEL;
  string_ids[1][1][1] = IDS_PAYMENTS_ACCEPTED_CARDS_LABEL;

  return l10n_util::GetStringUTF16(string_ids[credit][debit][prepaid]);
}

base::string16 GetCardTypesAreAcceptedText(
    const std::set<autofill::CreditCard::CardType>& types) {
  int credit =
      types.find(autofill::CreditCard::CARD_TYPE_CREDIT) != types.end() ? 1 : 0;
  int debit =
      types.find(autofill::CreditCard::CARD_TYPE_DEBIT) != types.end() ? 1 : 0;
  int prepaid =
      types.find(autofill::CreditCard::CARD_TYPE_PREPAID) != types.end() ? 1
                                                                         : 0;
  int string_ids[2][2][2];

  // [credit][debit][prepaid]
  string_ids[0][0][0] = -1;
  string_ids[0][0][1] = IDS_PAYMENTS_PREPAID_CARDS_ARE_ACCEPTED_LABEL;
  string_ids[0][1][0] = IDS_PAYMENTS_DEBIT_CARDS_ARE_ACCEPTED_LABEL;
  string_ids[0][1][1] = IDS_PAYMENTS_DEBIT_PREPAID_CARDS_ARE_ACCEPTED_LABEL;
  string_ids[1][0][0] = IDS_PAYMENTS_CREDIT_CARDS_ARE_ACCEPTED_LABEL;
  string_ids[1][0][1] = IDS_PAYMENTS_CREDIT_PREPAID_CARDS_ARE_ACCEPTED_LABEL;
  string_ids[1][1][0] = IDS_PAYMENTS_CREDIT_DEBIT_CARDS_ARE_ACCEPTED_LABEL;
  string_ids[1][1][1] = -1;

  return (credit == 0 && debit == 0 && prepaid == 0) ||
                 (credit == 1 && debit == 1 && prepaid == 1)
             ? base::string16()
             : l10n_util::GetStringUTF16(string_ids[credit][debit][prepaid]);
}

}  // namespace payments
