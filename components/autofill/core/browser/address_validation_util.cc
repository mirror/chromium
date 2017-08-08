// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/address_validation_util.h"

#include <utility>

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/address_i18n.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_data.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_validator.h"

namespace autofill {

namespace {

using ::i18n::addressinput::COUNTRY;
using ::i18n::addressinput::ADMIN_AREA;
using ::i18n::addressinput::LOCALITY;
using ::i18n::addressinput::DEPENDENT_LOCALITY;
using ::i18n::addressinput::SORTING_CODE;
using ::i18n::addressinput::POSTAL_CODE;
using ::i18n::addressinput::STREET_ADDRESS;
using ::i18n::addressinput::RECIPIENT;

using ::i18n::addressinput::AddressData;
using ::i18n::addressinput::AddressField;
using ::i18n::addressinput::AddressProblem;
using ::i18n::addressinput::FieldProblemMap;

using ::i18n::addressinput::INVALID_FORMAT;
using ::i18n::addressinput::MISMATCHING_VALUE;
using ::i18n::addressinput::MISSING_REQUIRED_FIELD;
using ::i18n::addressinput::UNEXPECTED_FIELD;
using ::i18n::addressinput::UNKNOWN_VALUE;

const AddressField kFields[] = {COUNTRY, ADMIN_AREA, POSTAL_CODE};
const AddressProblem kProblems[] = {UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD,
                                    UNKNOWN_VALUE, INVALID_FORMAT,
                                    MISMATCHING_VALUE};

// If the |address_field| is valid, set the validity state of the
// |address_field| in the |profile| to the |state| and return true.
// Otherwise, return false.
bool SetValidityStateForAddressField(AutofillProfile* profile,
                                     AddressField address_field,
                                     AutofillProfile::ValidityState state) {
  ServerFieldType server_field = i18n::TypeForField(address_field,
                                                    /*billing=*/false);
  if (server_field == UNKNOWN_TYPE)
    return false;
  DCHECK(profile);
  profile->SetValidityState(server_field, state);
  return true;
}

// Set the validity state of all address fields in the |profile| to |state|.
void SetAllValidityStates(AutofillProfile* profile,
                          AutofillProfile::ValidityState state) {
  DCHECK(profile);
  for (auto field : kFields)
    SetValidityStateForAddressField(profile, field, state);
}

// Returns all relevant pairs of (field, problem), where field is in
// |kFields|, and problem is in |kProblems|.
FieldProblemMap GetFilter() {
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

// Initializes |address| data from the address info in the |profile|.
void InitializeAddressFromProfile(AutofillProfile* profile,
                                  AddressData* address) {
  DCHECK(address);
  address->region_code =
      base::UTF16ToUTF8(profile->GetRawInfo(ADDRESS_HOME_COUNTRY));
  address->administrative_area =
      base::UTF16ToUTF8(profile->GetRawInfo(ADDRESS_HOME_STATE));
  address->postal_code =
      base::UTF16ToUTF8(profile->GetRawInfo(ADDRESS_HOME_ZIP));
}

}  // namespace

namespace address_validation_util {

AutofillProfile::ValidityState ValidateAddress(
    AutofillProfile* profile,
    AddressValidator* address_validator) {
  DCHECK(address_validator);
  if (!profile)
    return AutofillProfile::UNVALIDATED;
  AddressData address;
  InitializeAddressFromProfile(profile, &address);

  AutofillProfile::ValidityState profile_validity;
  // |filter| will make sure that the validation only returns problems that are
  // relevant.
  FieldProblemMap filter = GetFilter();
  FieldProblemMap problems;
  AddressValidator::Status status =
      address_validator->ValidateAddress(address, &filter, &problems);

  if (status == AddressValidator::SUCCESS) {
    profile_validity = AutofillProfile::VALID;
    SetAllValidityStates(profile, AutofillProfile::VALID);
  } else {
    // If the rules are not available, ValidateAddress can still check for
    // UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD. In this case, the address
    // fields are either UNVALIDATED or INVALID.

    // For now, an invalid country code is regarded as UNVALIDATED.
    // TODO(crbug/752539): Validate the country code.
    profile_validity = AutofillProfile::UNVALIDATED;
    SetAllValidityStates(profile, AutofillProfile::UNVALIDATED);
  }
  for (auto problem : problems) {
    if (SetValidityStateForAddressField(profile, problem.first,
                                        AutofillProfile::INVALID)) {
      profile_validity = AutofillProfile::INVALID;
    }
  }
  return profile_validity;
}

}  // namespace address_validation_util
}  // namespace autofill
