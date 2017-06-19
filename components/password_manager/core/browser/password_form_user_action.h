// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_USER_ACTION_H_
#define COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_USER_ACTION_H_

namespace password_manager {

// UserAction - What does the user do with a form? If they do nothing
// (either by accepting what the password manager did, or by simply (not
// typing anything at all), you get None). If there were multiple choices and
// the user selects one other than the default, you get Choose, if user
// selects an entry from matching against the Public Suffix List you get
// ChoosePslMatch, if the user types in a new value for just the password you
// get OverridePassword, and if the user types in a new value for the
// username and password you get OverrideUsernameAndPassword.
enum class UserAction {
  kUserActionNone = 0,
  kUserActionChoose,
  kUserActionChoosePslMatch,
  kUserActionOverridePassword,
  kUserActionOverrideUsernameAndPassword,
  kUserActionMax
};

}  // namespace password_manager

#endif  // COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_FORM_USER_ACTION_H_
