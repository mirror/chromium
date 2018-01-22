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

struct TestFieldData {
  bool is_password;
  bool is_focusable;
  bool is_empty;
  char* autocomplete_attribute = nullptr;
  // if |value| == nullptr, then field value will be set to default value. todo
  // write better.
  char* value = nullptr;
};

struct FormParsingTestCase {
  const char* description;
  std::vector<TestFieldData> fields;
  ParseResultIndices fill_result;
  ParseResultIndices save_result;
};

class FormParserTest : public testing::Test {
 public:
  FormParserTest() {}

 protected:
  FormParser form_parser_;

  void CheckTestData(const std::vector<FormParsingTestCase>& test_cases);
};

FormData GetFormData(const FormParsingTestCase& test_form) {
  FormData form_data;
  form_data.action = GURL("http://example1.com");
  form_data.origin = GURL("http://example2.com");
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

void FormParserTest::CheckTestData(
    const std::vector<FormParsingTestCase>& test_cases) {
  for (size_t i = 0; i < test_cases.size(); ++i) {
    const FormParsingTestCase& test_case = test_cases[i];
    const FormData form_data = GetFormData(test_case);
    for (auto mode : {FormParsingMode::FILLING, FormParsingMode::SAVING}) {
      SCOPED_TRACE(testing::Message("Index of test_case = ")
                   << i << ", parsing mode = "
                   << (mode == FormParsingMode::FILLING ? "Filling" : "Saving")
                   << "\n Test description: " << test_case.description);
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
      } else if (!expected_indices.IsEmpty() && parsed_form) {
        CheckPasswordFormFields(*parsed_form, form_data, expected_indices);
      } else {
        // Expected and parsed results are empty, everything is ok.
      }
    }
  }
}

TEST_F(FormParserTest, NonFormFields) {}

TEST_F(FormParserTest, FormFieldsNoAutocomplete) {
  std::vector<FormParsingTestCase> test_data = {
      {
          "Simple empty sign-in form",
          {{.is_password = false, .is_focusable = true, .is_empty = true},
           {.is_password = true, .is_focusable = true, .is_empty = true}},
          {0, 1, kFieldNotFound, kFieldNotFound},
          // Form with empty fields on saving does not make any sense, so empty
          // parsing.
          {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
      },
      {
          "Simple sign-in form with filled data",
          {{.is_password = false, .is_focusable = true, .is_empty = false},
           {.is_password = true, .is_focusable = true, .is_empty = false}},
          {0, 1, kFieldNotFound, kFieldNotFound},
          {0, 1, kFieldNotFound, kFieldNotFound},
      },
      {
          "Empty sign-in form with an extra text field",
          {{.is_password = false, .is_focusable = true, .is_empty = true},
           {.is_password = false, .is_focusable = true, .is_empty = true},
           {.is_password = true, .is_focusable = true, .is_empty = true}},
          {1, 2, kFieldNotFound, kFieldNotFound},
          {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
      },
      {
          "Non-empty sign-in form with an extra text field",
          {{.is_password = false, .is_focusable = true, .is_empty = false},
           {.is_password = false, .is_focusable = true, .is_empty = true},
           {.is_password = true, .is_focusable = true, .is_empty = false}},
          {1, 2, kFieldNotFound, kFieldNotFound},
          {0, 2, kFieldNotFound, kFieldNotFound},
      },
      {
          "Empty sign-in form with an extra invisible text field",
          {{.is_password = false, .is_focusable = true, .is_empty = true},
           {.is_password = false, .is_focusable = false, .is_empty = true},
           {.is_password = true, .is_focusable = true, .is_empty = true}},
          {0, 2, kFieldNotFound, kFieldNotFound},
          {kFieldNotFound, kFieldNotFound, kFieldNotFound, kFieldNotFound},
      },
      {
          "Non-empty sign-in form with an extra invisible text field",
          {{.is_password = false, .is_focusable = true, .is_empty = false},
           {.is_password = false, .is_focusable = false, .is_empty = false},
           {.is_password = true, .is_focusable = true, .is_empty = false}},
          {0, 2, kFieldNotFound, kFieldNotFound},
          {0, 2, kFieldNotFound, kFieldNotFound},
      },
      {
          "Simple empty sign-in form with empty username",
          {{.is_password = false, .is_focusable = true, .is_empty = true},
           {.is_password = true, .is_focusable = true, .is_empty = false}},
          {0, 1, kFieldNotFound, kFieldNotFound},
          // Form with empty username does not make sense, so username field
          // should not be found.
          {kFieldNotFound, 1, kFieldNotFound, kFieldNotFound},
      },
  };

  CheckTestData(test_data);
}

TEST_F(FormParserTest, FormFieldsWithAutocomplete) {}

}  // namespace
