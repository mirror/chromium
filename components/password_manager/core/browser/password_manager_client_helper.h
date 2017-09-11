// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_CLIENT_HELPER_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_CLIENT_HELPER_H_

#include "components/password_manager/core/browser/password_manager_client.h"

namespace password_manager {

// Delegate class for PasswordManagerClientHelper. A class that wants to use
// PasswordManagerClientHelper must implement this delegate.
class PasswordManagerClientHelperDelegate {
 public:
  // Shows the dialog where the user can accept or decline the global autosignin
  // setting as a first run experience. The dialog won't appear in Incognito or
  // when the autosign-in is off.
  virtual void PromptUserToEnableAutosigninIfNecessary() = 0;
};

// Helper class for PasswordManagerClients. It extracts some common logic for
// ChromePasswordManagerClient and IOSChromePasswordManagerClient.
class PasswordManagerClientHelper {
 public:
  explicit PasswordManagerClientHelper(
      PasswordManagerClientHelperDelegate* delegate);
  ~PasswordManagerClientHelper();

  // Implementation of PasswordManagerClient::NotifyUserCouldBeAutoSignedIn.
  void NotifyUserCouldBeAutoSignedIn(
      std::unique_ptr<autofill::PasswordForm> form);

  // Implementation of
  // PasswordManagerClient::NotifySuccessfulLoginWithExistingPassword.
  void NotifySuccessfulLoginWithExistingPassword(
      const autofill::PasswordForm& form);

 private:
  PasswordManagerClientHelperDelegate* delegate_;

  // Set during 'NotifyUserCouldBeAutoSignedIn' in order to store the
  // form for potential use during 'NotifySuccessfulLoginWithExistingPassword'.
  std::unique_ptr<autofill::PasswordForm> possible_auto_sign_in_;
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_CLIENT_HELPER_H_
