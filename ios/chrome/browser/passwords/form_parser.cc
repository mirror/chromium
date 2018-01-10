// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/form_parser.h"

#include <algorithm>

#include "base/strings/string_split.h"

using autofill::FormData;
using autofill::FormFieldData;
using autofill::PasswordForm;

using FieldPointersVector = std::vector<const FormFieldData*>;

namespace {
const char kAutocompleteUsername[] = "username";
const char kAutocompleteCurrentPassword[] = "current-password";
const char kAutocompleteNewPassword[] = "new-password";
const char kAutocompleteCreditCardPrefix[] = "cc-";

bool HasCreditCardAutocompleteAttributes(const FormFieldData& field) {
    std::vector<base::StringPiece> tokens =
    base::SplitStringPiece(field.autocomplete_attribute, base::kWhitespaceASCII,
                           base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    return std::find_if(
                        tokens.begin(), tokens.end(), [](base::StringPiece token) {
                            return base::StartsWith(token, kAutocompleteCreditCardPrefix,
                                                    base::CompareCase::INSENSITIVE_ASCII);
                        }) != tokens.end();
}

bool HasAutocompleteAttributeValue(const FormFieldData& field,
                                   base::StringPiece value_in_lowercase) {
    std::vector<base::StringPiece> tokens =
    base::SplitStringPiece(field.autocomplete_attribute, base::kWhitespaceASCII,
                           base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    
    return std::find_if(tokens.begin(), tokens.end(),
                        [value_in_lowercase](base::StringPiece token) {
                            return base::LowerCaseEqualsASCII(token,
                                                              value_in_lowercase);
                        }) != tokens.end();
}

bool AreTherePasswordFields(const std::vector<FormFieldData>& fields) {
    for (const auto& field : fields)
        if (field.form_control_type == "password")
            return true;
    return false;
}
    
FieldPointersVector NonCreditCardFields(const std::vector<FormFieldData>& fields) {
    FieldPointersVector result;
    for (sizt_t i = 0; i < fields.size(); ++i)
        if (!HasCreditCardAutocompleteAttributes(fields[i]))
            result.push_back(&fields[i]);
    return result;
}
    
std::unique_ptr<ParseResult> ParseUsingAutocomplete(
                const FieldPointersVector& fields) {
    size_t username_index = -1;
    size_t password_index = -1;
    std::vector<size_t> new_password_indices;
    for (size_t i = 0; i < fields.size(); ++i) {
        const auto& field = fields[i];
        if (HasAutocompleteAttributeValue(field, kAutocompleteUsername)) {
            if (username_index != -1)
                return nullptr;
            username_index = i;  // todo i or field_indices[i].
        } else if (HasAutocompleteAttributeValue(field, kAutocompleteCurrentPassword)) {
            if (username_index != -1)
                return nullptr;
            password_index = i;
        } if (HasAutocompleteAttributeValue(field, kAutocompleteNewPassword)) {
            new_password_indices.push_back(i);
        }
    }
    
    if (password_index == -1 && new_password_indices.empty())
        return nullptr;
    
    auto result = std::make_unique<ParseResult>();
    result->username_index = username_index;
    result->password_index = password_index;
    if (new_password_indices.size() > 0)
      result->new_password_index = new_password_indices[0];
    
    if (new_password_indices.size() > 1)
        result->confirmation_password_index = new_password_indices[1];
    return result;
}
    
void SetFields(const std::unique_ptr<ParseResult> parse_result, PasswordForm* result) {
    // todo
}
  
std::vector<size_t> FilterOutNotRelevantFields(
                                      const std::vector<FormFieldData>& fields,
                                      const std::vector<size_t>& field_indices,
                                      FormParsingMode mode) {
  return std::vector<size_t>(); // todo
}
    
    
std::unique_ptr<ParseResult> ParseUsingBaseHeuristics(
                                                    const std::vector<FormFieldData>& fields,
                                                    const std::vector<size_t>& field_indices) {
  return nullptr; // todo
}
    
    
}  // namespace

namespace passwords {
    
FormParser::FormParser() = default;

std::unique_ptr<PasswordForm> FormParser::Parse(const FormData& form_data,
                                                      FormParsingMode mode) {
    
    // Skip forms without password fields.
    if (!AreTherePasswordFields(form_data.fields))
        return nullptr;
    
    FieldPointersVector fields = NonCreditCardFields(fields);
    
    std::unique_ptr<PasswordForm> result = std::make_unique<PasswordForm>();
    
    auto autocomplete_parse_result = ParseUsingAutocomplete(fields, indices);
    if (autocomplete_parse_result) {
        SetFields(autocomplete_parse_result, result.get());
        return result;
    }
    
    indices = FilterOutNotRelevantFields(fields, indices);
    
    std::unique_ptr<PasswordForm> result = ParseUsingBaseHeuristics(fields, indices, mode);
    
    return result;
}
    
}  // namespace  passwords

