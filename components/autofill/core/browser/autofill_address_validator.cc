// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_address_validator.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_data.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_validator.h"

namespace autofill {

const AddressField AutofillAddressValidator::kFields[] = {COUNTRY, ADMIN_AREA,
                                                          POSTAL_CODE};
const AddressProblem AutofillAddressValidator::kProblems[] = {
    UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD, UNKNOWN_VALUE, INVALID_FORMAT,
    MISMATCHING_VALUE};

AutofillAddressValidator::AutofillAddressValidator(
    AutofillProfile* profile,
    AddressValidator* address_validator)
    : profile_(profile), address_validator_(address_validator) {}

AutofillAddressValidator::~AutofillAddressValidator() {}

AutofillProfile::ValidityState AutofillAddressValidator::ValidateAddress() {
  DCHECK(address_validator_);
  if (!profile_)
    return AutofillProfile::UNVALIDATED;

  AddressData address;
  InitializeAddressFromProfile(&address);

  AutofillProfile::ValidityState profile_validity;
  FieldProblemMap filter = get_filter();
  FieldProblemMap problems;
  AddressValidator::Status status =
      address_validator_->ValidateAddress(address, &filter, &problems);

  if (status == AddressValidator::SUCCESS) {
    profile_validity = AutofillProfile::VALID;
    SetAllValidityStates(AutofillProfile::VALID);
  } else {
    // If the rules are not available, ValidateAddress can still check for
    // UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD. In this case, the address
    // fields are either UNVALIDATED or INVALID.
    // For now, an invalid country code is regarded as UNVALIDATED.
    profile_validity = AutofillProfile::UNVALIDATED;
    SetAllValidityStates(AutofillProfile::UNVALIDATED);
  }
  for (auto problem : problems) {
    if (SetValidityStateForAddressField(problem.first,
                                        AutofillProfile::INVALID)) {
      profile_validity = AutofillProfile::INVALID;
    }
  }
  return profile_validity;
}

bool AutofillAddressValidator::SetValidityStateForAddressField(
    AddressField address_field,
    AutofillProfile::ValidityState state) const {
  ServerFieldType server_field = i18n::TypeForField(address_field, false);
  if (server_field == UNKNOWN_TYPE)
    return false;
  profile_->SetValidityState(server_field, state);
  return true;
}

void AutofillAddressValidator::SetAllValidityStates(
    AutofillProfile::ValidityState state) const {
  for (auto field : kFields)
    SetValidityStateForAddressField(field, state);
}

FieldProblemMap AutofillAddressValidator::get_filter() {
  static FieldProblemMap filter;
  if (!filter.empty())
    return filter;

  for (auto field : kFields) {
    for (auto problem : kProblems) {
      filter.insert(std::make_pair(field, problem));
    }
  }
  return filter;
}

void AutofillAddressValidator::InitializeAddressFromProfile(
    AddressData* address) {
  DCHECK(address);
  address->region_code =
      base::UTF16ToUTF8(profile_->GetRawInfo(ADDRESS_HOME_COUNTRY));
  address->administrative_area =
      base::UTF16ToUTF8(profile_->GetRawInfo(ADDRESS_HOME_STATE));
  address->postal_code =
      base::UTF16ToUTF8(profile_->GetRawInfo(ADDRESS_HOME_ZIP));
}

}  // namespace autofill
