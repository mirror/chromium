// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_ADDRESS_VALIDATOR_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_ADDRESS_VALIDATOR_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "components/autofill/core/browser/address_i18n.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "third_party/libaddressinput/chromium/chrome_address_validator.h"

namespace autofill {

namespace {

using ::i18n::addressinput::AddressData;
using ::i18n::addressinput::AddressField;
using ::i18n::addressinput::AddressProblem;
using ::i18n::addressinput::BuildCallback;
using ::i18n::addressinput::FieldProblemMap;

using ::i18n::addressinput::COUNTRY;
using ::i18n::addressinput::ADMIN_AREA;
using ::i18n::addressinput::LOCALITY;
using ::i18n::addressinput::DEPENDENT_LOCALITY;
using ::i18n::addressinput::SORTING_CODE;
using ::i18n::addressinput::POSTAL_CODE;
using ::i18n::addressinput::STREET_ADDRESS;
using ::i18n::addressinput::RECIPIENT;

using ::i18n::addressinput::INVALID_FORMAT;
using ::i18n::addressinput::MISMATCHING_VALUE;
using ::i18n::addressinput::MISSING_REQUIRED_FIELD;
using ::i18n::addressinput::UNEXPECTED_FIELD;
using ::i18n::addressinput::UNKNOWN_VALUE;

}  // namespace

class AutofillAddressValidator {
 public:
  AutofillAddressValidator(AutofillProfile* profile_,
                           AddressValidator* address_validator);
  ~AutofillAddressValidator();
  // Validates the address fields of the profile.
  AutofillProfile::ValidityState ValidateAddress();

 private:
  static const AddressProblem kProblems[];
  static const AddressField kFields[];
  // Not owned. Outlives this object.
  AutofillProfile* profile_;
  // Not owned. Never null. Outlives this object.
  AddressValidator* address_validator_;

  bool SetValidityStateForAddressField(
      AddressField address_field,
      AutofillProfile::ValidityState state) const;

  void SetAllValidityStates(AutofillProfile::ValidityState state) const;
  FieldProblemMap get_filter();

  void InitializeAddressFromProfile(AddressData* address);
  DISALLOW_COPY_AND_ASSIGN(AutofillAddressValidator);
};

}  // namespace autofill
#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_ADDRESS_VALIDATOR_H_
