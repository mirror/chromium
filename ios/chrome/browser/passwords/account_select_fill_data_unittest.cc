// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/account_select_fill_data.h"

#include "base/strings/utf_string_conversions.cc"
#include "components/autofill/core/common/password_form_fill_data.h"
#include "ios/chrome/browser/passwords/test_helpers.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using autofill::PasswordFormFillData;
using password_manager::AccountSelectFillData;
using password_manager::UsernameAndRealm;
using test_helpers::SetPasswordFormFillData;

namespace {
// Test data.
const char* url = "http://example.com/";
const char* form_names[] = {"form_name1", "form_name2"};
const char* username_elements[] = {"username1", "username2"};
const char* usernames[] = {"user0", "u_s_e_r"};
const char* password_elements[] = {"password1", "password2"};
const char* passwords[] = {"password0", "secret"};
const char* additional_usernames[] = {"user2", nullptr};
const char* additional_passwords[] = {"secret", nullptr};

class AccountSelectFillDataTest : public testing::Test {
 public:
  AccountSelectFillDataTest() {
    for (size_t i = 0; i < arraysize(form_data_); ++i) {
      SetPasswordFormFillData(form_data_[i], url, url, username_elements[i],
                              usernames[i], password_elements[i], passwords[i],
                              additional_usernames[i], additional_passwords[i],
                              false);

      form_data_[i].name = base::ASCIIToUTF16(form_names[i]);
    }
  }

 protected:
  PasswordFormFillData form_data_[2];
};

TEST_F(AccountSelectFillDataTest, EmptyReset) {
  AccountSelectFillData account_select_fill_data;
  EXPECT_TRUE(account_select_fill_data.Empty());

  account_select_fill_data.Add(form_data_[0]);
  EXPECT_FALSE(account_select_fill_data.Empty());

  account_select_fill_data.Reset();
  EXPECT_TRUE(account_select_fill_data.Empty());
}

TEST_F(AccountSelectFillDataTest, IsSuggestionsAvailableOneForm) {
  AccountSelectFillData account_select_fill_data;
  account_select_fill_data.Add(form_data_[0]);

  // Suggestions are avaialable for the correct form and field names.
  EXPECT_TRUE(account_select_fill_data.IsSuggestionsAvailable(
      form_data_[0].name, form_data_[0].username_field.name));
  // Suggestions are not avaialable for different form name.
  EXPECT_FALSE(account_select_fill_data.IsSuggestionsAvailable(
      form_data_[0].name + base::ASCIIToUTF16("1"),
      form_data_[0].username_field.name));

  // Suggestions are not avaialable for different field name.
  EXPECT_FALSE(account_select_fill_data.IsSuggestionsAvailable(
      form_data_[0].name,
      form_data_[0].username_field.name + base::ASCIIToUTF16("1")));
}

TEST_F(AccountSelectFillDataTest, IsSuggestionsAvailableTwoForms) {
  AccountSelectFillData account_select_fill_data;
  account_select_fill_data.Add(form_data_[0]);
  account_select_fill_data.Add(form_data_[1]);

  // Suggestions are avaialable for the correct form and field names.
  EXPECT_TRUE(account_select_fill_data.IsSuggestionsAvailable(
      form_data_[0].name, form_data_[0].username_field.name));
  // Suggestions are avaialable for the correct form and field names.
  EXPECT_TRUE(account_select_fill_data.IsSuggestionsAvailable(
      form_data_[1].name, form_data_[1].username_field.name));
  // Suggestions are not avaialable for different form name.
  EXPECT_FALSE(account_select_fill_data.IsSuggestionsAvailable(
      form_data_[0].name + base::ASCIIToUTF16("1"),
      form_data_[0].username_field.name));
}

TEST_F(AccountSelectFillDataTest, RetrieveSuggestionsOneForm) {
  AccountSelectFillData account_select_fill_data;
  account_select_fill_data.Add(form_data_[0]);

  std::vector<UsernameAndRealm> suggestions =
      account_select_fill_data.RetrieveSuggestions(
          form_data_[0].name, form_data_[0].username_field.name,
          base::string16());
  EXPECT_EQ(2u, suggestions.size());
  EXPECT_EQ(base::ASCIIToUTF16(usernames[0]), suggestions[0].username);
  EXPECT_EQ(base::ASCIIToUTF16(additional_usernames[0]),
            suggestions[1].username);
}

TEST_F(AccountSelectFillDataTest, RetrieveSuggestionsTwoForm) {
  // Test that after adding two PasswordFormFillData, suggestions for both forms
  // are shown, with credentials from the second PasswordFormFillData. That
  // emulates the case when credentials in the Password Store were changed
  // between load the first and the second forms.
  AccountSelectFillData account_select_fill_data;
  account_select_fill_data.Add(form_data_[0]);
  account_select_fill_data.Add(form_data_[1]);

  std::vector<UsernameAndRealm> suggestions =
      account_select_fill_data.RetrieveSuggestions(
          form_data_[0].name, form_data_[0].username_field.name,
          base::string16());
  EXPECT_EQ(1u, suggestions.size());
  EXPECT_EQ(base::ASCIIToUTF16(usernames[1]), suggestions[0].username);

  suggestions = account_select_fill_data.RetrieveSuggestions(
      form_data_[1].name, form_data_[1].username_field.name, base::string16());
  EXPECT_EQ(1u, suggestions.size());
  EXPECT_EQ(base::ASCIIToUTF16(usernames[1]), suggestions[0].username);
}

}  // namespace
