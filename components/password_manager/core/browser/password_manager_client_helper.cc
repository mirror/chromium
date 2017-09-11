// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_manager_client_helper.h"

namespace password_manager {

PasswordManagerClientHelper::PasswordManagerClientHelper(
    PasswordManagerClientHelperDelegate* delegate)
    : delegate_(delegate) {
  DCHECK(delegate_);
}

PasswordManagerClientHelper::~PasswordManagerClientHelper() {
  DCHECK(delegate_);
}

void PasswordManagerClientHelper::NotifyUserCouldBeAutoSignedIn(
    std::unique_ptr<autofill::PasswordForm> form) {
  possible_auto_sign_in_ = std::move(form);
}

void PasswordManagerClientHelper::NotifySuccessfulLoginWithExistingPassword(
    const autofill::PasswordForm& form) {
  if (!possible_auto_sign_in_)
    return;

  if (possible_auto_sign_in_->username_value == form.username_value &&
      possible_auto_sign_in_->password_value == form.password_value &&
      possible_auto_sign_in_->origin == form.origin) {
    delegate_->PromptUserToEnableAutosigninIfNecessary();
  }
  possible_auto_sign_in_.reset();
}

}  // namespace password_manager
