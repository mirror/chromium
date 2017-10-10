// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/phone_email_validation_util.h"

#include <string>

#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/browser/autofill_test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill {

class AutofillPhoneValidationTest : public testing::Test {
 public:
  AutofillPhoneValidationTest() {}

  void ValidatePhoneAndEmailTest(AutofillProfile* profile) {
    phone_email_validation_util::ValidatePhoneAndEmail(profile);
  }

  ~AutofillPhoneValidationTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(AutofillPhoneValidationTest);
};

TEST_F(AutofillPhoneValidationTest, ValidateFullValidProfile) {
  // This is a full valid profile:
  // Country Code: "CA", Phone Number: "15141112233",
  // Email: "alice@wonderland.ca"
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));
}

TEST_F(AutofillPhoneValidationTest, ValidateEmptyPhoneNumber) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, base::string16());
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::EMPTY,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));
}

TEST_F(AutofillPhoneValidationTest, ValidateValidPhone_CountryCodeNotExist) {
  // This is a profile with invalid country code, therefore the phone number
  // cannot be validated.
  const std::string country_code = "PP";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_COUNTRY, base::UTF8ToUTF16(country_code));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::UNVALIDATED,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));
}

TEST_F(AutofillPhoneValidationTest, ValidateEmptyPhone_CountryCodeNotExist) {
  // This is a profile with invalid country code, but a missing phone number.
  // Therefore, it's an invalid phone number.
  const std::string country_code = "PP";
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(ADDRESS_HOME_COUNTRY, base::UTF8ToUTF16(country_code));
  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, base::string16());
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::EMPTY,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));
}

TEST_F(AutofillPhoneValidationTest, ValidateInvalidPhoneNumber) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, base::ASCIIToUTF16("33"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("151411122334"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("1(514)111-22-334"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("251411122334"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, base::ASCIIToUTF16("Hello!"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
}

TEST_F(AutofillPhoneValidationTest, ValidateValidPhoneNumber) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER, base::ASCIIToUTF16("5141112233"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("514-111-2233"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("1(514)111-22-33"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("+1 514 111 22 33"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("+1 (514)-111-22-33"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("(514)-111-22-33"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("+1 650 GOO OGLE"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));

  profile.SetRawInfo(PHONE_HOME_WHOLE_NUMBER,
                     base::ASCIIToUTF16("778 111 22 33"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
}

TEST_F(AutofillPhoneValidationTest, ValidateEmptyEmailAddress) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(EMAIL_ADDRESS, base::string16());
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID,
            profile.GetValidityState(PHONE_HOME_WHOLE_NUMBER));
  EXPECT_EQ(AutofillProfile::EMPTY, profile.GetValidityState(EMAIL_ADDRESS));
}

TEST_F(AutofillPhoneValidationTest, ValidateInvalidEmailAddress) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());
  profile.SetRawInfo(EMAIL_ADDRESS, base::ASCIIToUTF16("Hello!"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID, profile.GetValidityState(EMAIL_ADDRESS));

  profile.SetRawInfo(EMAIL_ADDRESS, base::ASCIIToUTF16("alice.wonderland"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID, profile.GetValidityState(EMAIL_ADDRESS));

  profile.SetRawInfo(EMAIL_ADDRESS, base::ASCIIToUTF16("alice@"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID, profile.GetValidityState(EMAIL_ADDRESS));

  profile.SetRawInfo(EMAIL_ADDRESS,
                     base::ASCIIToUTF16("alice@=wonderland.com"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::INVALID, profile.GetValidityState(EMAIL_ADDRESS));
}

TEST_F(AutofillPhoneValidationTest, ValidateValidEmailAddress) {
  AutofillProfile profile(autofill::test::GetFullValidProfile());

  profile.SetRawInfo(EMAIL_ADDRESS, base::ASCIIToUTF16("alice@wonderland"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));

  profile.SetRawInfo(EMAIL_ADDRESS,
                     base::ASCIIToUTF16("alice@wonderland.fiction"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));

  profile.SetRawInfo(EMAIL_ADDRESS,
                     base::ASCIIToUTF16("alice+cat@wonderland.fiction.book"));
  ValidatePhoneAndEmailTest(&profile);
  EXPECT_EQ(AutofillProfile::VALID, profile.GetValidityState(EMAIL_ADDRESS));
}

}  // namespace autofill
