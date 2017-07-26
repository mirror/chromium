// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_profile_validator.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/cancelable_callback.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_data.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/source.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/storage.h"
#include "third_party/libaddressinput/src/cpp/src/rule.h"

namespace autofill {
namespace {

static constexpr AddressField kFields[] = {COUNTRY, ADMIN_AREA, POSTAL_CODE};
static constexpr AddressProblem kProblems[] = {
    UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD, UNKNOWN_VALUE, INVALID_FORMAT,
    MISMATCHING_VALUE};

class ValidationRequest : public AutofillProfileValidator::Request {
 public:
  ValidationRequest(AutofillProfile* profile,
                    autofill::AddressValidator* address_validator,
                    AutofillProfileValidatorCallback on_validated)
      : profile_(profile),
        address_validator_(address_validator),
        on_validated_(std::move(on_validated)),
        has_responded_(false),
        on_timeout_(base::Bind(&::autofill::ValidationRequest::OnRulesLoaded,
                               base::Unretained(this),
                               false)) {
    if (!profile_) {
      // An empty profile is an invalid profile.
      std::move(on_validated_).Run(AutofillProfile::UNVALIDATED);
      return;
    }
    InitializeFilter();
    base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, on_timeout_.callback(), base::TimeDelta::FromSeconds(5));
  }

  ~ValidationRequest() override { on_timeout_.Cancel(); }

  void OnRulesLoaded(bool success) override {
    // I may need success in future.

    on_timeout_.Cancel();
    // Check if the timeout happened before the rules were loaded.
    if (has_responded_)
      return;
    has_responded_ = true;

    // std::string language = profile_->language_code();
    // empty unless set_language is used.

    AddressData address;
    InitializeAddress(&address);

    AutofillProfile::ValidityState profile_validity = ValidateAddress(address);
    std::move(on_validated_).Run(profile_validity);
  }

 private:
  // Not owned. Outlives this object.
  AutofillProfile* profile_;
  // Not owned. Never null. Outlives this object.
  autofill::AddressValidator* address_validator_;

  AutofillProfileValidatorCallback on_validated_;

  bool has_responded_;
  base::CancelableCallback<void()> on_timeout_;

  FieldProblemMap filter;

  AutofillProfile::ValidityState ValidateAddress(const AddressData& address) {
    // if (!RegionDataConstants::IsSupported(address.region_code)) {
    //   // Country not valid.
    //   SetAllValidityStates(AutofillProfile::UNVALIDATED);
    //   SetValidityState(COUNTRY, AutofillProfile::INVALID);
    //   return AutofillProfile::INVALID; // Can't check for
    //   MISSING_REQUIRED_FIELD.
    // }

    AutofillProfile::ValidityState profile_validity;
    FieldProblemMap problems;
    AddressValidator::Status status =
        address_validator_->ValidateAddress(address, &filter, &problems);

    if (status == AddressValidator::SUCCESS) {
      profile_validity = AutofillProfile::VALID;
      SetAllValidityStates(AutofillProfile::VALID);
    } else {
      // For now, invalid country code is taken as unvalidated.
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

  bool SetValidityStateForAddressField(
      AddressField address_field,
      AutofillProfile::ValidityState state) const {
    ServerFieldType server_field = i18n::TypeForField(address_field, false);
    if (server_field == UNKNOWN_TYPE)
      return false;
    profile_->SetValidityState(server_field, state);
    return true;
  }

  void SetAllValidityStates(AutofillProfile::ValidityState state) const {
    for (auto field : kFields)
      SetValidityStateForAddressField(field, state);
  }

  void InitializeFilter() {
    for (auto field : kFields) {
      for (auto problem : kProblems) {
        filter.insert(std::make_pair(field, problem));
      }
    }
  }

  void InitializeAddress(AddressData* address) {
    DCHECK(address);
    address->region_code =
        base::UTF16ToUTF8(profile_->GetRawInfo(ADDRESS_HOME_COUNTRY));
    address->administrative_area =
        base::UTF16ToUTF8(profile_->GetRawInfo(ADDRESS_HOME_STATE));
    address->postal_code =
        base::UTF16ToUTF8(profile_->GetRawInfo(ADDRESS_HOME_ZIP));
  }

  DISALLOW_COPY_AND_ASSIGN(ValidationRequest);
};

}  // namespace

AutofillProfileValidator::AutofillProfileValidator(
    std::unique_ptr<Source> source,
    std::unique_ptr<Storage> storage)
    : address_validator_(std::move(source), std::move(storage), this) {}

AutofillProfileValidator::~AutofillProfileValidator() {}

void AutofillProfileValidator::ValidateProfile(
    AutofillProfile* profile,
    AutofillProfileValidatorCallback cb) {
  std::unique_ptr<ValidationRequest> request(
      base::MakeUnique<ValidationRequest>(profile, &address_validator_,
                                          std::move(cb)));
  if (!profile)
    return;
  std::string region_code =
      base::UTF16ToUTF8(profile->GetRawInfo(ADDRESS_HOME_COUNTRY));
  if (address_validator_.AreRulesLoadedForRegion(region_code)) {
    request->OnRulesLoaded(true);
  } else {
    // Setup the variables so that the subkeys request is sent, when the rules
    // are loaded.
    pending_rule_code = region_code;
    pending_request_ = std::move(request);

    // Start loading the rules for that region. If the rules were already in the
    // process of being loaded, this call will do nothing.
    address_validator_.LoadRules(region_code);
  }
}
void AutofillProfileValidator::OnAddressValidationRulesLoaded(
    const std::string& region_code,
    bool success) {
  // if success = 0, then we still need to validate the address, because we can
  // still check for UNEXPECTED_FIELD, MISSING_REQUIRED_FIELD.
  // Check if there is any request for that region code.
  if (pending_request_ && !pending_rule_code.compare(region_code)) {
    pending_request_->OnRulesLoaded(success);
  }
  pending_rule_code.clear();
  pending_request_.reset();
}
}  // namespace autofill
