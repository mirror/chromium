// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/autofill_profile_validation_util.h"

#include <memory>
#include <string>

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libaddressinput/src/cpp/include/libaddressinput/null_storage.h"
#include "third_party/libaddressinput/src/cpp/test/testdata_source.h"

namespace autofill {

namespace {

using ::i18n::addressinput::Source;
using ::i18n::addressinput::Storage;
using ::i18n::addressinput::NullStorage;
using ::i18n::addressinput::TestdataSource;

}  // namespace

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

class AutofillProfileValidationUtilTest : public testing::Test,
                                          LoadRulesListener {
 public:
  AutofillProfileValidationUtilTest()
      : validator_(std::unique_ptr<Source>(new ValidationTestDataSource()),
                   std::unique_ptr<Storage>(new NullStorage),
                   this) {
    validator_.LoadRules("CA");
  }

  void ValidateProfileTest(AutofillProfile* profile) {
    profile_validation_util::ValidateProfile(profile, &validator_);
  }

  void ValidateAddressTest(AutofillProfile* profile) {
    profile_validation_util::ValidateAddress(profile, &validator_);
  }

  void ValidatePhoneTest(AutofillProfile* profile) {
    profile_validation_util::ValidatePhoneNumber(profile);
  }

  void ValidateEmailTest(AutofillProfile* profile) {
    profile_validation_util::ValidateEmailAddress(profile);
  }

  ~AutofillProfileValidationUtilTest() override {}

 private:
  AddressValidator validator_;

  // LoadRulesListener implementation.
  void OnAddressValidationRulesLoaded(const std::string& country_code,
                                      bool success) override {}

  DISALLOW_COPY_AND_ASSIGN(AutofillProfileValidationUtilTest);
};

TEST_F(AutofillProfileValidationUtilTest, ValidateFullValidProfile) {
  // This is a valid profile according to the rules in ValidationTestDataSource:
  // Address Line 1: "666 Notre-Dame Ouest",
  // Address Line 2: "Apt 8", City: "Montreal", Province: "QC",
  // Postal Code: "H3B 2T9", Country Code: "CA",
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  ValidateAddressTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillProfileValidationUtilTest,
       ValidateFullProfile_CountryCodeNotExist) {
  // This is a profile with invalid country code, therefore it cannot be
  // validated according to ValidationTestDataSource.
  const std::string country_code = "PP";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_COUNTRY, base::UTF8ToUTF16(country_code));
  ValidateAddressTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::UNVALIDATED,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::UNVALIDATED,
            profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillProfileValidationUtilTest,
       ValidateFullProfile_EmptyCountryCode) {
  // This is a profile with no country code, therefore it cannot be validated
  // according to ValidationTestDataSource.
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_COUNTRY, base::UTF8ToUTF16(""));
  ValidateAddressTest(&profile);
  EXPECT_EQ(AutofillProfile::EMPTY,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::UNVALIDATED,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::UNVALIDATED,
            profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillProfileValidationUtilTest,
       ValidateFullProfile_RuleNotAvailable) {
  // This is a profile with valid country code, but the rule is not available in
  // the ValidationTestDataSource.
  const std::string country_code = "US";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_COUNTRY, base::UTF8ToUTF16(country_code));
  ValidateAddressTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::UNVALIDATED,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::UNVALIDATED,
            profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillProfileValidationUtilTest, ValidateAddress_AdminAreaNotExists) {
  const std::string admin_area_code = "QQ";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_STATE, base::UTF8ToUTF16(admin_area_code));

  ValidateAddressTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillProfileValidationUtilTest, ValidateAddress_EmptyAdminArea) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_STATE, base::UTF8ToUTF16(""));

  ValidateAddressTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::EMPTY,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillProfileValidationUtilTest, ValidateAddress_AdminAreaFullName) {
  const std::string admin_area = "Quebec";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_STATE, base::UTF8ToUTF16(admin_area));

  ValidateAddressTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillProfileValidationUtilTest, ValidateAddress_AdminAreaSmallCode) {
  const std::string admin_area = "qc";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_STATE, base::UTF8ToUTF16(admin_area));

  ValidateAddressTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillProfileValidationUtilTest,
       ValidateAddress_AdminAreaSpecialLetter) {
  const std::string admin_area = "Québec";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_STATE, base::UTF8ToUTF16(admin_area));

  ValidateAddressTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillProfileValidationUtilTest, ValidateAddress_ValidZipNoSpace) {
  // TODO(crbug/752614): postal codes in lower case letters should also be
  // considered as valid. Now, they are considered as INVALID.
  const std::string postal_code = "H3C6S3";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_ZIP, base::UTF8ToUTF16(postal_code));

  ValidateAddressTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillProfileValidationUtilTest, ValidateAddress_InvalidZip) {
  const std::string postal_code = "ABC 123";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_ZIP, base::UTF8ToUTF16(postal_code));

  ValidateAddressTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillProfileValidationUtilTest, ValidateAddress_EmptyZip) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_ZIP, base::UTF8ToUTF16(""));

  ValidateAddressTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::EMPTY, profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillProfileValidationUtilTest,
       ValidateAddressProfile_EmptyCountryCodeAreaAndZip) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_COUNTRY, base::UTF8ToUTF16(""));
  profile.SetRawInfo(ADDRESS_HOME_STATE, base::UTF8ToUTF16(""));
  profile.SetRawInfo(ADDRESS_HOME_ZIP, base::UTF8ToUTF16(""));
  ValidateAddressTest(&profile);
  EXPECT_EQ(AutofillProfile::EMPTY,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::EMPTY,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::EMPTY, profile.GetValidityState(ADDRESS_HOME_ZIP));
}

TEST_F(AutofillProfileValidationUtilTest, ValidatePhone_FullValidProfile) {
  // This is a full valid profile:
  // Country Code: "CA", Phone Number: "15141112233"
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
}

TEST_F(AutofillProfileValidationUtilTest, ValidatePhone_EmptyPhoneNumber) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, base::string16());
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::EMPTY,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
}

TEST_F(AutofillProfileValidationUtilTest,
       ValidatePhone_ValidPhoneCountryCodeNotExist) {
  // This is a profile with invalid country code, therefore the phone number
  // cannot be validated.
  const std::string country_code = "PP";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_COUNTRY, base::UTF8ToUTF16(country_code));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::UNVALIDATED,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
}

TEST_F(AutofillProfileValidationUtilTest,
       ValidatePhone_EmptyPhoneCountryCodeNotExist) {
  // This is a profile with invalid country code, but a missing phone number.
  // Therefore, it's an invalid phone number.
  const std::string country_code = "PP";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_COUNTRY, base::UTF8ToUTF16(country_code));
  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, base::string16());
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::EMPTY,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
}

TEST_F(AutofillProfileValidationUtilTest, ValidatePhone_InvalidPhoneNumber) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, base::ASCIIToUTF16("33"));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("151411122334"));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("1(514)111-22-334"));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("251411122334"));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, base::ASCIIToUTF16("Hello!"));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
}

TEST_F(AutofillProfileValidationUtilTest, ValidatePhone_ValidPhoneNumber) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, base::ASCIIToUTF16("5141112233"));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("514-111-2233"));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("1(514)111-22-33"));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("+1 514 111 22 33"));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("+1 (514)-111-22-33"));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("(514)-111-22-33"));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("+1 650 GOO OGLE"));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("778 111 22 33"));
  ValidatePhoneTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
}

TEST_F(AutofillProfileValidationUtilTest, ValidateEmail_FullValidProfile) {
  // This is a full valid profile:
  // Email: "alice@wonderland.ca"
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  ValidateEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));
}

TEST_F(AutofillProfileValidationUtilTest, ValidateEmail_EmptyEmailAddress) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(EMAIL_ADDRESS, base::string16());
  ValidateEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::EMPTY, profile.GetValidityState(EMAIL_ADDRESS));
}

TEST_F(AutofillProfileValidationUtilTest,
       ValidateEmail_ValidateInvalidEmailAddress) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(EMAIL_ADDRESS, base::ASCIIToUTF16("Hello!"));
  ValidateEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID, profile.GetValidityState(EMAIL_ADDRESS));

  profile.SetRawInfo(EMAIL_ADDRESS, base::ASCIIToUTF16("alice.wonderland"));
  ValidateEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID, profile.GetValidityState(EMAIL_ADDRESS));

  profile.SetRawInfo(EMAIL_ADDRESS, base::ASCIIToUTF16("alice@"));
  ValidateEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID, profile.GetValidityState(EMAIL_ADDRESS));

  profile.SetRawInfo(EMAIL_ADDRESS,
                     base::ASCIIToUTF16("alice@=wonderland.com"));
  ValidateEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID, profile.GetValidityState(EMAIL_ADDRESS));
}

TEST_F(AutofillProfileValidationUtilTest, ValidateEmail_ValidEmailAddress) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());

  profile.SetRawInfo(EMAIL_ADDRESS, base::ASCIIToUTF16("alice@wonderland"));
  ValidateEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));

  profile.SetRawInfo(EMAIL_ADDRESS,
                     base::ASCIIToUTF16("alice@wonderland.fiction"));
  ValidateEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));

  profile.SetRawInfo(EMAIL_ADDRESS,
                     base::ASCIIToUTF16("alice+cat@wonderland.fiction.book"));
  ValidateEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));
}

TEST_F(AutofillProfileValidationUtilTest, ValidateProfile_FullValidProfile) {
  // This is a full valid profile:
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  ValidateProfileTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(ADDRESS_HOME_ZIP));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));
}

TEST_F(AutofillProfileValidationUtilTest,
       ValidateProfile_FullValidProfileWithInvalidZip) {
  // This is a full valid profile:
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_ZIP, base::UTF8ToUTF16("ABC 123"));
  ValidateProfileTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(ADDRESS_HOME_ZIP));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));
}

TEST_F(AutofillProfileValidationUtilTest,
       ValidateProfile_FullValidProfileWithInvalidPhone) {
  // This is a full valid profile:
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, base::ASCIIToUTF16("33"));
  ValidateProfileTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(ADDRESS_HOME_ZIP));
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));
}

TEST_F(AutofillProfileValidationUtilTest,
       ValidateProfile_FullValidProfileWithInvalidEmail) {
  // This is a full valid profile:
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(EMAIL_ADDRESS, base::ASCIIToUTF16("fakeaddress"));
  ValidateProfileTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_COUNTRY));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(ADDRESS_HOME_STATE));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(ADDRESS_HOME_ZIP));
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
  EXPECT_EQ(AutofillProfile::INVALID, profile.GetValidityState(EMAIL_ADDRESS));
}

// TODO(crbug/754727): add tests for a non-default language.
// Ex: Nouveau-Brunswick for Canada.

// TODO(crbug/754729): Add tests for a country whose default language is a
// non-Western one, such as China.

}  // namespace autofill
