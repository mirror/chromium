// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/form_parser.h"

#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "components/autofill/core/common/form_data.h"
#include "components/autofill/core/common/form_field_data.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using autofill::FormData;
using autofill::FormFieldData;
using autofill::PasswordForm;
using base::ASCIIToUTF16;
using base::UintToString16;
using passwords::FormParser;
using passwords::FormParsingMode;

namespace {

class FormParserTest : public testing::Test {
 public:
  FormParserTest() {}

 protected:
  FormParser form_parser_;
};

TEST_F(FormParserTest, NonFormFields) {}

struct TestFieldData {
  bool is_password;
  bool is_focusable;
  bool is_empty;
  char* autocomplete_attribute = nullptr;
};

const int kFieldNotFound = -1;
struct ParseResultIndices {
  int username_index;
  int password_index;
  int new_password_index;
  int confirmation_password_index;

  bool IsEmpty() const {
    return username_index == kFieldNotFound &&
           password_index == kFieldNotFound &&
           new_password_index == kFieldNotFound &&
           confirmation_password_index == kFieldNotFound;
  }
};

struct TestFormData {
  const char* description;
  std::vector<TestFieldData> fields;
  ParseResultIndices fill_result;
  ParseResultIndices save_result;
};

FormData GetFormData(const TestFormData& test_form) {
  FormData form_data;
  form_data.action = GURL("example1.com");
  form_data.origin = GURL("example2.com");
  for (size_t i = 0; i < test_form.fields.size(); ++i) {
    const TestFieldData& field_data = test_form.fields[i];
    FormFieldData field;
    field.id = ASCIIToUTF16("field_id") + UintToString16(i);
    field.form_control_type = field_data.is_password ? "password" : "text";
    field.is_focusable = field_data.is_focusable;
    if (!field_data.is_empty)
      field.value = ASCIIToUTF16("field_value") + UintToString16(i);
    form_data.fields.push_back(field);
  }
  return form_data;
}

void CheckField(const std::vector<FormFieldData>& fields,
                int field_index,
                const base::string16* element,
                const base::string16* value) {
  // todo add scope
  base::string16 expected_element;
  base::string16 expected_value;
  if (field_index != kFieldNotFound) {
    const FormFieldData& field = fields[field_index];
    expected_element = field.id;
    expected_value = field.value;
  }
  EXPECT_EQ(expected_element, *element);
  if (value)
    EXPECT_EQ(expected_value, *value);
}

void CheckPasswordFormFields(const PasswordForm& password_form,
                             const FormData& form_data,
                             const ParseResultIndices& expected_fields) {
  CheckField(form_data.fields, expected_fields.username_index,
             &password_form.username_element, &password_form.username_value);

  CheckField(form_data.fields, expected_fields.password_index,
             &password_form.password_element, &password_form.password_value);

  CheckField(form_data.fields, expected_fields.new_password_index,
             &password_form.new_password_element,
             &password_form.new_password_value);

  CheckField(form_data.fields, expected_fields.confirmation_password_index,
             &password_form.confirmation_password_element, nullptr);

  // Check username field.
}

TEST_F(FormParserTest, FormFieldsNoAutocomplete) {
  TestFormData test_data[] = {{
      "Simple sign-in form",
      {{true, true, true}, {false, true, true}},
      {0, 1, kFieldNotFound, kFieldNotFound},
      {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
  }};

  for (size_t i = 0; i < arraysize(test_data); ++i) {
    const TestFormData& test_case = test_data[i];
    const FormData form_data = GetFormData(test_case);
    for (auto mode : {FormParsingMode::FILLING, FormParsingMode::SAVING}) {
      // todo add scope
      std::unique_ptr<PasswordForm> parsed_form =
          form_parser_.Parse(form_data, mode);

      const ParseResultIndices& expected_indices =
          mode == FormParsingMode::FILLING ? test_case.fill_result
                                           : test_case.save_result;

      if (expected_indices.IsEmpty() != (parsed_form == nullptr)) {
        if (expected_indices.IsEmpty())
          EXPECT_TRUE(false) << "Expected no parsed results";
        else
          EXPECT_TRUE(false) << "The form is expected to be parsed";
      } else {
        CheckPasswordFormFields(*parsed_form, form_data, expected_indices);
      }
    }
  }
}

TEST_F(FormParserTest, FormFieldsWithAutocomplete) {}

}  // namespace
