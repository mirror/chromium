// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_CLIENT_HELPER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_CLIENT_HELPER_H_

#include "components/password_manager/core/browser/password_manager_client.h"

namespace password_manager {

class PasswordManagerClientHelper {
 public:
  explicit PasswordManagerClientHelper(PasswordManagerClient* client);
  ~PasswordManagerClientHelper();

  void NotifyUserCouldBeAutoSignedIn(
      std::unique_ptr<autofill::PasswordForm> form);

  void NotifySuccessfulLoginWithExistingPassword(
      const autofill::PasswordForm& form);

 private:
  PasswordManagerClient* client_;

  // Set during 'NotifyUserCouldBeAutoSignedIn' in order to store the
  // form for potential use during 'NotifySuccessfulLoginWithExistingPassword'.
  std::unique_ptr<autofill::PasswordForm> possible_auto_sign_in_;
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_CLIENT_HELPER_H_
