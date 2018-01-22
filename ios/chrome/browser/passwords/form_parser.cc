// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/passwords/form_parser.h"

#include <algorithm>

#include "base/strings/string_split.h"

using autofill::FormData;
using autofill::FormFieldData;
using autofill::PasswordForm;
using passwords::FormParsingMode;

using FieldPointersVector = std::vector<const FormFieldData*>;

namespace {
const char kAutocompleteUsername[] = "username";
const char kAutocompleteCurrentPassword[] = "current-password";
const char kAutocompleteNewPassword[] = "new-password";
const char kAutocompleteCreditCardPrefix[] = "cc-";

struct ParseResult {
  const FormFieldData* username_field = nullptr;
  const FormFieldData* password_field = nullptr;
  const FormFieldData* new_password_field = nullptr;
  const FormFieldData* confirmation_password_field = nullptr;

  bool IsEmpty() {  // todo: do we need this function?
    return password_field == nullptr && new_password_field == nullptr;
  }
};

bool HasCreditCardAutocompleteAttributes(const FormFieldData& field) {
  std::vector<base::StringPiece> tokens = base::SplitStringPiece(
      field.autocomplete_attribute, base::kWhitespaceASCII,
      base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  return std::find_if(
             tokens.begin(), tokens.end(), [](base::StringPiece token) {
               return base::StartsWith(token, kAutocompleteCreditCardPrefix,
                                       base::CompareCase::INSENSITIVE_ASCII);
             }) != tokens.end();
}

bool HasAutocompleteAttributeValue(const FormFieldData& field,
                                   base::StringPiece value_in_lowercase) {
  std::vector<base::StringPiece> tokens = base::SplitStringPiece(
      field.autocomplete_attribute, base::kWhitespaceASCII,
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

FieldPointersVector GetNonCreditCardFields(
    const std::vector<FormFieldData>& fields) {
  FieldPointersVector result;
  for (size_t i = 0; i < fields.size(); ++i)
    if (!HasCreditCardAutocompleteAttributes(fields[i]))
      result.push_back(&fields[i]);
  return result;
}

std::unique_ptr<ParseResult> ParseUsingAutocomplete(
    const FieldPointersVector& fields) {
  std::unique_ptr<ParseResult> result = std::make_unique<ParseResult>();
  for (size_t i = 0; i < fields.size(); ++i) {
    const auto* field = fields[i];
    if (HasAutocompleteAttributeValue(*field, kAutocompleteUsername)) {
      if (result->username_field)
        return nullptr;
      result->username_field = field;
    } else if (HasAutocompleteAttributeValue(*field,
                                             kAutocompleteCurrentPassword)) {
      if (result->password_field)
        return nullptr;
      result->password_field = field;
    }
    if (HasAutocompleteAttributeValue(*field, kAutocompleteNewPassword)) {
      if (!result->new_password_field)
        result->new_password_field = field;
      else if (!result->confirmation_password_field)
        result->confirmation_password_field = field;
      else
        return nullptr;
    }
  }

  return result->IsEmpty() ? nullptr : std::move(result);
}

FieldPointersVector FilterFields(const FieldPointersVector& fields,
                                 bool is_password,
                                 bool only_focusable,
                                 bool only_non_empty) {
  FieldPointersVector result;
  for (auto* field : fields) {
    if ((field->form_control_type == "password") != is_password)
      continue;
    if (only_focusable && !field->is_focusable)
      continue;
    if (only_non_empty && field->value.empty())
      continue;
    result.push_back(field);
  }
  return result;
}

FieldPointersVector GetRelevantPasswords(const FieldPointersVector& fields,
                                         FormParsingMode mode) {
  // Focusable is proxy for visibility. todo write more.
  bool is_there_focusable_password_field =
      std::any_of(fields.begin(), fields.end(), [](const auto* field) {
        return field->is_focusable && field->form_control_type == "password";
      });

  return FilterFields(fields, true, is_there_focusable_password_field,
                      mode == FormParsingMode::SAVING);
}

void LocateSpecificPasswords(const FieldPointersVector& passwords,
                             const FormFieldData** current_password,
                             const FormFieldData** new_password,
                             const FormFieldData** confirmation_password) {
  switch (passwords.size()) {
    case 1:
      // Single password, easy.
      *current_password = passwords[0];
      break;
    case 2:
      if (!passwords[0]->value.empty() &&
          passwords[0]->value == passwords[1]->value) {
        // Two identical non-empty passwords: assume we are seeing a new
        // password with a confirmation. This can be either a sign-up form or a
        // password change form that does not ask for the old password.
        *new_password = passwords[0];
        *confirmation_password = passwords[1];
      } else {
        // Assume first is old password, second is new (no choice but to guess).
        // This case also includes empty passwords in order to allow filling of
        // password change forms (that also could autofill for sign up form, but
        // we can't do anything with this using only client side information).
        *current_password = passwords[0];
        *new_password = passwords[1];
      }
      break;
    default:
      if (!passwords[0]->value.empty() &&
          passwords[0]->value == passwords[1]->value &&
          passwords[0]->value == passwords[2]->value) {
        // All three passwords are the same and non-empty? It may be a change
        // password form where old and new passwords are the same. It doesn't
        // matter what field is correct, let's save the value.
        *current_password = passwords[0];
      } else if (passwords[1]->value == passwords[2]->value) {
        // New password is the duplicated one, and comes second; or empty form
        // with 3 password fields, in which case we will assume this layout.
        *current_password = passwords[0];
        *new_password = passwords[1];
        *confirmation_password = passwords[2];
      } else if (passwords[0]->value == passwords[1]->value) {
        // It is strange that the new password comes first, but trust more which
        // fields are duplicated than the ordering of fields. Assume that
        // any password fields after the new password contain sensitive
        // information that isn't actually a password (security hint, SSN, etc.)
        *new_password = passwords[0];
        *confirmation_password = passwords[1];
      } else {
        // Three different passwords, or first and last match with middle
        // different. No idea which is which. Let's save the first password.
        // Password selection in a prompt will allow to correct the choice.
        *current_password = passwords[0];
      }
  }
}

bool HasFocusable(const FieldPointersVector& fields) {
  for (auto* field : fields) {
    if (field->is_focusable)
      return true;
  }
  return false;
}

const FormFieldData* FindUsernameField(
    const FieldPointersVector& fields,
    const FormFieldData* first_relevant_password,
    FormParsingMode mode) {
  DCHECK(first_relevant_password);
  auto first_relevant_password_it =
      std::find(fields.begin(), fields.end(), first_relevant_password);
  DCHECK(fields.end() != first_relevant_password_it);
  FieldPointersVector candidates(fields.begin(), first_relevant_password_it);
  bool consider_only_non_empty = mode == FormParsingMode::SAVING;
  candidates =
      FilterFields(fields, false /* is_password */, false /* only_focusable */,
                   consider_only_non_empty /* only_non_empty */);
  bool consider_only_focusable = HasFocusable(fields);
  for (auto field_iter = candidates.rbegin(); field_iter != candidates.rend();
       ++field_iter) {
    if (!consider_only_focusable || (*field_iter)->is_focusable)
      return *field_iter;
  }
  return nullptr;
}

std::unique_ptr<ParseResult> ParseUsingBaseHeuristics(
    const FieldPointersVector& fields,
    passwords::FormParsingMode mode) {
  FieldPointersVector passwords = GetRelevantPasswords(fields, mode);
  if (passwords.empty())
    return nullptr;

  std::unique_ptr<ParseResult> result = std::make_unique<ParseResult>();
  LocateSpecificPasswords(passwords, &result->password_field,
                          &result->new_password_field,
                          &result->confirmation_password_field);
  if (result->IsEmpty())
    return nullptr;

  result->username_field = FindUsernameField(fields, passwords[0], mode);
  return result;
}

void SetFields(const ParseResult& parse_result, PasswordForm* result) {
  if (parse_result.username_field) {
    result->username_element = parse_result.username_field->id;
    result->username_value = parse_result.username_field->value;
  }

  if (parse_result.password_field) {
    result->password_element = parse_result.password_field->id;
    result->password_value = parse_result.password_field->value;
  }

  if (parse_result.new_password_field) {
    result->new_password_element = parse_result.password_field->id;
    result->new_password_value = parse_result.password_field->value;
  }

  if (parse_result.confirmation_password_field) {
    result->confirmation_password_element =
        parse_result.confirmation_password_field->id;
  }
}

}  // namespace

namespace passwords {

FormParser::FormParser() = default;

std::unique_ptr<PasswordForm> FormParser::Parse(const FormData& form_data,
                                                FormParsingMode mode) {
  // Skip forms without password fields.
  if (!AreTherePasswordFields(form_data.fields))
    return nullptr;

  FieldPointersVector fields = GetNonCreditCardFields(form_data.fields);

  std::unique_ptr<PasswordForm> result = std::make_unique<PasswordForm>();
  result->origin = form_data.origin;
  result->signon_realm = form_data.origin.GetOrigin().spec();
  result->action = form_data.action;

  auto autocomplete_parse_result = ParseUsingAutocomplete(fields);
  if (autocomplete_parse_result) {
    SetFields(*autocomplete_parse_result, result.get());
    return result;
  }

  auto base_heuristics_parse_result = ParseUsingBaseHeuristics(fields, mode);

  if (!base_heuristics_parse_result)
    return nullptr;

  SetFields(*base_heuristics_parse_result, result.get());
  return result;
}

}  // namespace  passwords
