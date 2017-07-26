// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_LIBADDRESSINPUT_CHROMIUM_AUTOFILL_PROFILE_VALIDATOR_H_
#define THIRD_PARTY_LIBADDRESSINPUT_CHROMIUM_AUTOFILL_PROFILE_VALIDATOR_H_

#include <stddef.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "components/autofill/core/browser/address_i18n.h"
#include "components/autofill/core/browser/autofill_profile.h"
#include "third_party/libaddressinput/chromium/chrome_address_validator.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_validator.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/preload_supplier.h"
#include "third_party/libaddressinput/src/cpp/src/rule.h"

namespace i18n {
namespace addressinput {
class Source;
class Storage;
struct AddressData;
}  // namespace addressinput
}  // namespace i18n

namespace autofill {

namespace {

using ::i18n::addressinput::AddressData;
using ::i18n::addressinput::AddressField;
using ::i18n::addressinput::AddressProblem;
using ::i18n::addressinput::BuildCallback;
using ::i18n::addressinput::FieldProblemMap;
using ::i18n::addressinput::PreloadSupplier;
using ::i18n::addressinput::Source;
using ::i18n::addressinput::Storage;

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

// This receives a region code and the device's language.
using AutofillProfileValidatorCallback =
    base::OnceCallback<void(AutofillProfile::ValidityState)>;
// code is in

class AutofillProfileValidator : public autofill::LoadRulesListener {
 public:
  // The interface for the AutofillProfileValidator request.
  class Request {
   public:
    virtual void OnRulesLoaded(bool success) = 0;
    virtual ~Request() {}
  };

  // Takes ownership of |source| and |storage|.
  AutofillProfileValidator(
      std::unique_ptr<::i18n::addressinput::Source> source,
      std::unique_ptr<::i18n::addressinput::Storage> storage);

  ~AutofillProfileValidator() override;

  void ValidateProfile(AutofillProfile* profile,
                       AutofillProfileValidatorCallback cb);
  void AddressValidationRulesLoaded(const std::string& region_code,
                                    bool success);

 private:
  // Called when the address rules for the |region_code| have finished
  // loading. Implementation of the LoadRulesListener interface.
  void OnAddressValidationRulesLoaded(const std::string& region_code,
                                      bool success) override;

  // The region code and the request for the pending subkey request.
  std::unique_ptr<Request> pending_request_;
  std::string pending_rule_code;

  // The address validator used to load rules.
  autofill::AddressValidator address_validator_;

  DISALLOW_COPY_AND_ASSIGN(AutofillProfileValidator);
};

}  // namespace autofill

#endif  // THIRD_PARTY_LIBADDRESSINPUT_CHROMIUM_AUTOFILL_PROFILE_VALIDATOR_H_
