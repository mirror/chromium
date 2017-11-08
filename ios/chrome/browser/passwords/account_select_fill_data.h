// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_ACCOUNT_SELECT_FILL_DATA_H_
#define IOS_CHROME_BROWSER_PASSWORDS_ACCOUNT_SELECT_FILL_DATA_H_

#include <map>
#include <string>
#include <vector>

namespace autofill {
class PasswordFormFillData;
}

namespace passwords {

struct FieldFillData {
  std::string name;
  std::string value;
};

struct FillData {
  std::string origin;
  std::string action;
  std::vector<FieldFillData> fields;
};

struct FormData {
  GURL action;
  std::string name;
  std::string username_element;
  std::string password_element;
};

struct Credential {
  base::string16 username;
  base::string16 password;
};

class AccountSelectFillData {
 public:
  void Add(const autofill::PasswordFormFillData& form_data);
  void Reset();
  void IsSuggestionsAvailable(const std::string& form_name,
                              const std::string& field_name,
                              const std::string& type,
                              const std::string& typed_value) const;

  std::vector<std::string> RetrieveSuggestions(const std::string& form_name,
                                               const std::string& field_name,
                                               const std::string& type,
                                               const std::string& typed_value);

  FillData GetFillData(const std::string& username);

 private:
  std::map<pair<std::string, std::string>, FormData> forms_;
  std::vector<Credential> credentials_;

  DISALLOW_COPY_AND_ASSIGN(AccountSelectFillData);
};

}  // namespace  passwords

#endif  // IOS_CHROME_BROWSER_PASSWORDS_ACCOUNT_SELECT_FILL_DATA_H_
