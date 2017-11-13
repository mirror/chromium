// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_PASSWORDS_ACCOUNT_SELECT_FILL_DATA_H_
#define IOS_CHROME_BROWSER_PASSWORDS_ACCOUNT_SELECT_FILL_DATA_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "url/gurl.h"

namespace autofill {
struct PasswordFormFillData;
}

namespace password_manager {

struct UsernameAndRealm {
  base::string16 username;
  std::string realm;
};

struct FormInfo {
  FormInfo();
  ~FormInfo();
  GURL origin;
  GURL action;
  base::string16 name;
  base::string16 username_element;
  base::string16 password_element;
};

struct Credential {
  Credential(const base::string16& username,
             const base::string16& password,
             const std::string& realm);
  ~Credential();
  base::string16 username;
  base::string16 password;
  std::string realm;
};

struct FillData {
  FillData();
  ~FillData();

  GURL origin;
  GURL action;
  base::string16 username_element;
  base::string16 username_value;
  base::string16 password_element;
  base::string16 password_value;
};

class AccountSelectFillData {
 public:
  AccountSelectFillData();
  ~AccountSelectFillData();

  void Add(const autofill::PasswordFormFillData& form_data);
  void Reset();
  bool Empty();
  bool IsSuggestionsAvailable(const base::string16& form_name,
                              const base::string16& field_name) const;

  std::vector<UsernameAndRealm> RetrieveSuggestions(
      const base::string16& form_name,
      const base::string16& field_name,
      const base::string16& typed_value) const;

  //  NSArray* BuildSuggestions() { return nil; };
  std::unique_ptr<FillData> GetFillData(const base::string16& username);

 private:
  // <form_name, username field_name> -> FormInfo
  std::map<std::pair<base::string16, base::string16>, FormInfo> forms_;
  std::vector<Credential> credentials_;
  mutable const FormInfo* last_requested_form_ = nullptr;

  const FormInfo* GetForm(const base::string16& form_name,
                          const base::string16& field_name) const;
  DISALLOW_COPY_AND_ASSIGN(AccountSelectFillData);
};

}  // namespace  password_manager

#endif  // IOS_CHROME_BROWSER_PASSWORDS_ACCOUNT_SELECT_FILL_DATA_H_
