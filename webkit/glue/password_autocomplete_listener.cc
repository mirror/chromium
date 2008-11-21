// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file provides the implementaiton of the password manager's autocomplete 
// component.

#include "webkit/glue/password_autocomplete_listener.h"
#include "base/logging.h"

namespace webkit_glue {

PasswordAutocompleteListener::PasswordAutocompleteListener(
    const std::wstring& initial_username_text,
    AutocompleteEditDelegateFactory* username_delegate_factory,
    AutocompleteEditDelegateFactory* password_delegate_factory,
    const PasswordFormDomManager::FillData& data)
    : AutocompleteInputListener(initial_username_text,
                                username_delegate_factory),
      password_delegate_factory_(password_delegate_factory),
      data_(data) {
}

void PasswordAutocompleteListener::OnBlur(const std::wstring& user_input) {
  // If this listener exists, its because the password manager had more than
  // one match for the password form, which implies it had at least one
  // [preferred] username/password pair.
  DCHECK(data_.basic_data.values.size() == 2);

  // Set the password field to match the current username.
  if (data_.basic_data.values[0] == user_input) {
    // Preferred username/login is selected.
    CurPasswordDelegate()->SetValue(data_.basic_data.values[1]);
  } else if (data_.additional_logins.find(user_input) !=
             data_.additional_logins.end()) {
    // One of the extra username/logins is selected.
    CurPasswordDelegate()->SetValue(data_.additional_logins[user_input]);
  }
  CurPasswordDelegate()->OnFinishedAutocompleting();
}

void PasswordAutocompleteListener::OnInlineAutocompleteNeeded(
    const std::wstring& user_input) {
  // If wait_for_username is true, we only autofill the password when
  // the username field is blurred (i.e not inline) with a matching
  // username string entered.
  if (data_.wait_for_username)
    return;
  // Look for any suitable matches to current field text.
  // TODO(timsteele): The preferred login (in basic_data.values) and
  // additional logins could be bundled into the same data structure
  // (possibly even as WebCore strings) upon construction of the
  // PasswordAutocompleteListener to simplify lookup and save string
  // conversions (see SetValue) on each successful call to
  // OnInlineAutocompleteNeeded.
  if (TryToMatch(user_input,
                 data_.basic_data.values[0],
                 data_.basic_data.values[1])) {
    return;
  }

  // Scan additional logins for a match.
  for (PasswordFormDomManager::LoginCollection::iterator it =
           data_.additional_logins.begin();
       it != data_.additional_logins.end();
       ++it) {
    if (TryToMatch(user_input, it->first, it->second))
      return;
  }
}

void PasswordAutocompleteListener::DidHandleEvent(WebCore::Event* event) {
  current_password_delegate_.reset();
  AutocompleteInputListener::DidHandleEvent(event);
}

bool PasswordAutocompleteListener::TryToMatch(const std::wstring& input,
                                              const std::wstring& username,
                                              const std::wstring& password) {
  if (input.compare(0, input.length(), username, 0, input.length()) != 0)
    return false;

  // Input matches the username, fill in required values.
  CurEditDelegate()->SetValue(username);
  CurEditDelegate()->SetSelectionRange(input.length(), username.length());
  CurEditDelegate()->OnFinishedAutocompleting();
  CurPasswordDelegate()->SetValue(password);
  CurPasswordDelegate()->OnFinishedAutocompleting();
  return true;
}

AutocompleteEditDelegate* PasswordAutocompleteListener::CurPasswordDelegate() {
  if (!current_password_delegate_.get()) {
    current_password_delegate_.reset(
        password_delegate_factory_->Create(current_event()));
  }
  return current_password_delegate_.get();
}

}  // webkit_glue

