// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_profile_validator.h"

#include <stddef.h>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/guid.h"
#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_data.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_problem.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/address_ui.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/null_storage.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/source.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/storage.h"
#include "third_party/libaddressinput/src/cpp/test/testdata_source.h"

namespace autofill {

using ::i18n::addressinput::NullStorage;
using ::i18n::addressinput::TestdataSource;

class AutofillProfileValidatorTest : public testing::Test {
 public:
  AutofillProfileValidatorTest()
      : validator_(new AutofillProfileValidator(
            std::unique_ptr<Source>(new TestdataSource(true)),
            std::unique_ptr<Storage>(new NullStorage))),
        onvalidated_cb_(
            base::BindOnce(&AutofillProfileValidatorTest::OnValidated,
                           base::Unretained(this))) {}

 protected:
  ~AutofillProfileValidatorTest() override {}

  void OnValidated(AutofillProfile::ValidityState profile_valid) {
    EXPECT_EQ(profile_valid_, profile_valid);
    if (!profile_)
      return;
    EXPECT_EQ(country_valid_, profile_->GetValidityState(ADDRESS_HOME_COUNTRY));
    EXPECT_EQ(admin_area_valid_,
              profile_->GetValidityState(ADDRESS_HOME_STATE));
    EXPECT_EQ(zip_valid_, profile_->GetValidityState(ADDRESS_HOME_ZIP));
  }

  void set_expected_status(AutofillProfile::ValidityState profile_valid,
                           AutofillProfile::ValidityState country_valid,
                           AutofillProfile::ValidityState admin_area_valid,
                           AutofillProfile::ValidityState zip_valid) {
    profile_valid_ = profile_valid;
    country_valid_ = country_valid;
    admin_area_valid_ = admin_area_valid;
    zip_valid_ = zip_valid;
  }
  const std::unique_ptr<AutofillProfileValidator> validator_;
  ::autofill::AutofillProfileValidatorCallback onvalidated_cb_;

  void set_profile(AutofillProfile* profile) { profile_ = profile; }

 private:
  // Not owned. Outlives this object.
  AutofillProfile* profile_;
  AutofillProfile::ValidityState profile_valid_;
  AutofillProfile::ValidityState country_valid_;
  AutofillProfile::ValidityState admin_area_valid_;
  AutofillProfile::ValidityState zip_valid_;

  base::test::ScopedTaskEnvironment scoped_task_scheduler;
  DISALLOW_COPY_AND_ASSIGN(AutofillProfileValidatorTest);
};

TEST_F(AutofillProfileValidatorTest, ValidateNullProfile) {
  set_profile(NULL);

  AutofillProfile::ValidityState profile_valid = AutofillProfile::UNVALIDATED;
  AutofillProfile::ValidityState country_valid = AutofillProfile::UNVALIDATED;
  AutofillProfile::ValidityState admin_area_valid =
      AutofillProfile::UNVALIDATED;
  AutofillProfile::ValidityState zip_valid = AutofillProfile::UNVALIDATED;
  set_expected_status(profile_valid, country_valid, admin_area_valid,
                      zip_valid);

  validator_->ValidateProfile(NULL, std::move(onvalidated_cb_));
}

TEST_F(AutofillProfileValidatorTest, ValidateAddress_MissingFields) {
  const std::string country_code = "CA";
  AutofillProfile profile(base::GenerateGUID(), "http://www.example.com/");
  profile.SetRawInfo(ADDRESS_HOME_COUNTRY, base::UTF8ToUTF16(country_code));
  set_profile(&profile);

  AutofillProfile::ValidityState profile_valid = AutofillProfile::INVALID;
  AutofillProfile::ValidityState country_valid = AutofillProfile::VALID;
  AutofillProfile::ValidityState admin_area_valid = AutofillProfile::INVALID;
  AutofillProfile::ValidityState zip_valid = AutofillProfile::INVALID;
  set_expected_status(profile_valid, country_valid, admin_area_valid,
                      zip_valid);

  validator_->ValidateProfile(&profile, std::move(onvalidated_cb_));
}

TEST_F(AutofillProfileValidatorTest, ValidateFullProfile) {
  // This is the valid profile:
  // Name: "Alice Wonderland", email: "alice@wonderland.ca" Company: "Fiction",
  // Address line 1: "666 Notre-Dame Ouest", Address line 2: "Apt 8",
  // City: "Montreal", Province: "QC", Country: "CA", Postal Code: "H3B 2T9",
  // Phone: "15141112233"
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  set_profile(&profile);

  AutofillProfile::ValidityState profile_valid = AutofillProfile::VALID;
  AutofillProfile::ValidityState country_valid = AutofillProfile::VALID;
  AutofillProfile::ValidityState admin_area_valid = AutofillProfile::VALID;
  AutofillProfile::ValidityState zip_valid = AutofillProfile::VALID;
  set_expected_status(profile_valid, country_valid, admin_area_valid,
                      zip_valid);

  validator_->ValidateProfile(&profile, std::move(onvalidated_cb_));
}

// When country code is invalid, can't load rules, would say unvalidated.
// TODO ?
TEST_F(AutofillProfileValidatorTest, ValidateAddress_CountryCodeNotExists) {
  const std::string country_code = "QQ";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_COUNTRY, base::UTF8ToUTF16(country_code));
  set_profile(&profile);

  AutofillProfile::ValidityState profile_valid = AutofillProfile::UNVALIDATED;
  AutofillProfile::ValidityState country_valid = AutofillProfile::UNVALIDATED;
  AutofillProfile::ValidityState admin_area_valid =
      AutofillProfile::UNVALIDATED;
  AutofillProfile::ValidityState zip_valid = AutofillProfile::UNVALIDATED;
  set_expected_status(profile_valid, country_valid, admin_area_valid,
                      zip_valid);

  validator_->ValidateProfile(&profile, std::move(onvalidated_cb_));
}

TEST_F(AutofillProfileValidatorTest, ValidateAddress_AdminAreaNotExists) {
  const std::string admin_area_code = "QQ";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_STATE, base::UTF8ToUTF16(admin_area_code));
  set_profile(&profile);

  AutofillProfile::ValidityState profile_valid = AutofillProfile::INVALID;
  AutofillProfile::ValidityState country_valid = AutofillProfile::VALID;
  AutofillProfile::ValidityState admin_area_valid = AutofillProfile::INVALID;
  AutofillProfile::ValidityState zip_valid = AutofillProfile::VALID;
  set_expected_status(profile_valid, country_valid, admin_area_valid,
                      zip_valid);

  validator_->ValidateProfile(&profile, std::move(onvalidated_cb_));
}

TEST_F(AutofillProfileValidatorTest, ValidateAddress_AdminAreaFullName) {
  const std::string admin_area = "Quebec";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_STATE, base::UTF8ToUTF16(admin_area));
  set_profile(&profile);

  AutofillProfile::ValidityState profile_valid = AutofillProfile::VALID;
  AutofillProfile::ValidityState country_valid = AutofillProfile::VALID;
  AutofillProfile::ValidityState admin_area_valid = AutofillProfile::VALID;
  AutofillProfile::ValidityState zip_valid = AutofillProfile::VALID;
  set_expected_status(profile_valid, country_valid, admin_area_valid,
                      zip_valid);

  validator_->ValidateProfile(&profile, std::move(onvalidated_cb_));
}

TEST_F(AutofillProfileValidatorTest, ValidateAddress_InvalidZip) {
  const std::string postal_code = "ABC 123";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_ZIP, base::UTF8ToUTF16(postal_code));
  set_profile(&profile);

  AutofillProfile::ValidityState profile_valid = AutofillProfile::INVALID;
  AutofillProfile::ValidityState country_valid = AutofillProfile::VALID;
  AutofillProfile::ValidityState admin_area_valid = AutofillProfile::VALID;
  AutofillProfile::ValidityState zip_valid = AutofillProfile::INVALID;
  set_expected_status(profile_valid, country_valid, admin_area_valid,
                      zip_valid);

  validator_->ValidateProfile(&profile, std::move(onvalidated_cb_));
}

// TODO after the change in Normalize is in.
TEST_F(AutofillProfileValidatorTest,
       ValidateAddress_AdminAreaFullName_Language) {
  const std::string admin_area = "Nouveau-Brunswick";
  const std::string locale = "fr_CA";  // fr--CA fr-CA none worked.
  // Validate should take care of language, and SetInfo should set the lang by
  // default.

  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.set_language_code(locale);  // ?
  profile.SetInfo(AutofillType(ADDRESS_HOME_STATE),
                  base::UTF8ToUTF16(admin_area), locale);
  set_profile(&profile);

  AutofillProfile::ValidityState profile_valid =
      AutofillProfile::INVALID;  // Now can't validate other languages.
  AutofillProfile::ValidityState country_valid = AutofillProfile::VALID;
  AutofillProfile::ValidityState admin_area_valid =
      AutofillProfile::INVALID;  // ?
  AutofillProfile::ValidityState zip_valid = AutofillProfile::VALID;
  set_expected_status(profile_valid, country_valid, admin_area_valid,
                      zip_valid);

  validator_->ValidateProfile(&profile, std::move(onvalidated_cb_));
}

}  // namespace autofill
