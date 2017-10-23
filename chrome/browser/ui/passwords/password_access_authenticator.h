// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_PASSWORDS_PASSWORD_ACCESS_AUTHENTICATOR_H_
#define CHROME_BROWSER_UI_PASSWORDS_PASSWORD_ACCESS_AUTHENTICATOR_H_

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/time/clock.h"
#include "base/time/time.h"

// This class takes care of reauthentication used for accessing passwords
// through the settings page. It is used on all platforms but iOS (see
// //ios/chrome/browser/ui/settings/reauthentication_module.* for iOS).
class PasswordAccessAuthenticator {
 public:
  // |os_reauth_call| is passed to |os_reauth_call_|, see the latter for
  // explanation.
  explicit PasswordAccessAuthenticator(
      base::RepeatingCallback<bool()> os_reauth_call);

  ~PasswordAccessAuthenticator();

  // Returns whether the user is able to pass the authentication challenge (see
  // |os_reauth_call_|). As a side effect, the user might see the corresponding
  // challenge UI. To avoid spamming the user, passing the challenge is
  // considered valid for 60 seconds and the user is not re-challenged again
  // within that time since the last successful attempt.
  bool EnsureUserIsAuthenticated();

  // Use this in tests to mock the OS-level reauthentication.
  void set_os_reauth_call_for_testing(
      base::RepeatingCallback<bool()> os_reauth_call) {
    os_reauth_call_ = std::move(os_reauth_call);
  }

  // Use this to manipulate time in tests.
  void set_clock_for_testing(std::unique_ptr<base::Clock> clock) {
    clock_ = std::move(clock);
  }

 private:
  // The last time the user was successfully authenticated.
  base::Optional<base::Time> last_authentication_time_;

  // Used to measure the time since the last authentication.
  std::unique_ptr<base::Clock> clock_;

  // Used to directly present the authentication challenge (such as the login
  // prompt) to the user.
  base::RepeatingCallback<bool()> os_reauth_call_;

  DISALLOW_COPY_AND_ASSIGN(PasswordAccessAuthenticator);
};

#endif  // CHROME_BROWSER_UI_PASSWORDS_PASSWORD_ACCESS_AUTHENTICATOR_H_
