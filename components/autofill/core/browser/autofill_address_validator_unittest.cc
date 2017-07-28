// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_address_validator.h"

#include <stddef.h>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/guid.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_task_environment.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/null_storage.h"
#include "third_party/libaddressinput/src/cpp/test/testdata_source.h"

namespace autofill {

using ::i18n::addressinput::Source;
using ::i18n::addressinput::Storage;
using ::i18n::addressinput::NullStorage;
using ::i18n::addressinput::TestdataSource;

// Used to load region rules for this test.
class ValidationTestDataSource : public TestdataSource {
 public:
  ValidationTestDataSource() : TestdataSource(true) {}

  ~ValidationTestDataSource() override {}

  void Get(const std::string& key, const Callback& data_ready) const override {
    data_ready(
        true, key,
        new std::string(
            "{"
            "\"data/CA\": "
            "{\"lang\": \"en\", \"upper\": \"ACNOSZ\", "
            "\"zipex\": \"H3Z 2Y7,V8X 3X4,T0L 1K0,T0H 1A0\", "
            "\"name\": \"CANADA\", "
            "\"fmt\": \"%N%n%O%n%A%n%C %S %Z\", \"id\": \"data/CA\", "
            "\"languages\": \"en~fr\", \"sub_keys\": \"NB~QC\", \"key\": "
            "\"CA\", "
            "\"require\": \"ACSZ\", \"sub_names\": \"New Brunswick~Quebec\", "
            "\"sub_zips\": \"E~G|H|J\"}, "
            "\"data/CA--fr\": "
            "{\"lang\": \"fr\", \"upper\": \"ACNOSZ\", "
            "\"zipex\": \"H3Z 2Y7,V8X 3X4,T0L 1K0,T0H 1A0\", "
            "\"name\": \"CANADA\", "
            "\"fmt\": \"%N%n%O%n%A%n%C %S %Z\", \"require\": \"ACSZ\", "
            "\"sub_keys\": \"NB~QC\", \"key\": \"CA\", "
            "\"id\": \"data/CA--fr\", "
            "\"sub_names\":\"Nouveau-Brunswick~Québec\","
            "\"sub_zips\": \"E~G|H|J\"}, "
            "\"data/CA/QC\": "
            "{\"lang\": \"en\", \"key\": \"QC\", "
            "\"id\": \"data/CA/QC\", \"zip\": \"G|H|J\", \"name\": \"Quebec\"},"
            "\"data/CA/QC--fr\": "
            "{\"lang\": \"fr\", \"key\": \"QC\", \"id\": \"data/CA/QC--fr\", "
            "\"zip\": \"G|H|J\", \"name\": \"Québec\"}, "
            "\"data/CA/NB\": "
            "{\"lang\": \"en\", \"key\": \"NB\", \"id\": \"data/CA/NB\", "
            "\"zip\": \"E\", \"name\": \"New Brunswick\"}, "
            "\"data/CA/NB--fr\": "
            "{\"lang\": \"fr\", \"key\": \"NB\", \"id\": \"data/CA/NB--fr\", "
            "\"zip\": \"E\", \"name\": \"Nouveau-Brunswick\"}"
            "}"));
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ValidationTestDataSource);
};

class AutofillAddressValidatorTest : public testing::Test, LoadRulesListener {
 public:
  AutofillAddressValidatorTest()
      : validator_(new AddressValidator(
            std::unique_ptr<Source>(new ValidationTestDataSource()),
            std::unique_ptr<Storage>(new NullStorage),
            this)) {
    validator_->LoadRules("CA");
  }

  AutofillProfile::ValidityState ValidateAddressTest(AutofillProfile* profile) {
    AutofillAddressValidator autofill_address_validator(profile, validator_);
    return autofill_address_validator.ValidateAddress();
  }

  ~AutofillAddressValidatorTest() override {}

 private:
  // owned. Never Null.
  AddressValidator* validator_;

  // LoadRulesListener implementation.
  void OnAddressValidationRulesLoaded(const std::string& country_code,
                                      bool success) override {}

  DISALLOW_COPY_AND_ASSIGN(AutofillAddressValidatorTest);
};

TEST_F(AutofillAddressValidatorTest, ValidateNULLProfile) {
  EXPECT_EQ(AutofillProfile::UNVALIDATED, ValidateAddressTest(NULL));
}

TEST_F(AutofillAddressValidatorTest, ValidateFullValidProfile) {
  // This is the valid profile:
  // Name: "Alice Wonderland", email: "alice@wonderland.ca" Company: "Fiction",
  // Address line 1: "666 Notre-Dame Ouest", Address line 2: "Apt 8",
  // City: "Montreal", Province: "QC", Country: "CA", Postal Code: "H3B 2T9",
  // Phone: "15141112233"
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  EXPECT_EQ(AutofillProfile::VALID, ValidateAddressTest(&profile));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillAddressValidatorTest, ValidateFullProfile_RulesNotLoaded) {
  // This is a US profile, therefore the rules are not loaded.
  AutofillProfile profile(autofill::test::GetFullProfile());
  EXPECT_EQ(AutofillProfile::UNVALIDATED, ValidateAddressTest(&profile));
  EXPECT_EQ(AutofillProfile::UNVALIDATED,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::UNVALIDATED,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::UNVALIDATED,
            profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillAddressValidatorTest, ValidateAddress_AdminAreaNotExists) {
  const std::string admin_area_code = "QQ";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_STATE, base::UTF8ToUTF16(admin_area_code));

  EXPECT_EQ(AutofillProfile::INVALID, ValidateAddressTest(&profile));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillAddressValidatorTest, ValidateAddress_AdminAreaFullName) {
  const std::string admin_area = "Quebec";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_STATE, base::UTF8ToUTF16(admin_area));

  EXPECT_EQ(AutofillProfile::VALID, ValidateAddressTest(&profile));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillAddressValidatorTest, ValidateAddress_InvalidZip) {
  const std::string postal_code = "ABC 123";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_ZIP, base::UTF8ToUTF16(postal_code));

  EXPECT_EQ(AutofillProfile::INVALID, ValidateAddressTest(&profile));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(ADDRESS_HOME_ZIP));
}

}  // namespace autofill
