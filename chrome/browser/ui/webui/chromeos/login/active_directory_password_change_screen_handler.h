// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ACTIVE_DIRECTORY_PASSWORD_CHANGE_SCREEN_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ACTIVE_DIRECTORY_PASSWORD_CHANGE_SCREEN_HANDLER_H_

#include <string>

#include "base/macros.h"
#include "chrome/browser/ui/webui/chromeos/login/base_screen_handler.h"
#include "chromeos/dbus/authpolicy/active_directory_info.pb.h"

namespace authpolicy {
class ActiveDirectoryAccountInfo;
}

namespace chromeos {

class AuthPolicyLoginHelper;
class Key;
class SigninScreenHandlerDelegate;

// A class that handles WebUI hooks in Active Directory password change  screen.
class ActiveDirectoryPasswordChangeScreenHandler : public BaseScreenHandler {
 public:
  ActiveDirectoryPasswordChangeScreenHandler();
  ~ActiveDirectoryPasswordChangeScreenHandler() override;

  // BaseScreenHandler implementation:
  void DeclareLocalizedValues(
      ::login::LocalizedValuesBuilder* builder) override;
  void Initialize() override;

  // WebUIMessageHandler implementation:
  void RegisterMessages() override;

  // WebUI message handlers.
  void HandleCompleteAdPasswordChange(const std::string& username,
                                      const std::string& old_password,
                                      const std::string& new_password);
  void HandleCancel();

  // Shows the password change screen for |username|. Uses |delegate| to
  // compelete authentication process.
  void ShowScreen(const std::string& username,
                  SigninScreenHandlerDelegate* delegate);

 private:
  // Shows the screen with the error message corresponding to |error|.
  void ShowScreenWithError(int error);

  // Callback for AuthPolicyClient.
  void DoAdAuth(const std::string& username,
                const Key& key,
                authpolicy::ErrorType error,
                const authpolicy::ActiveDirectoryAccountInfo& account_info);

  // Helper to call AuthPolicyClient and cancel calls if needed. Used to change
  // password on the Active Directory server.
  std::unique_ptr<AuthPolicyLoginHelper> authpolicy_login_helper_;

  // Non-owning ptr to SigninScreenHandlerDelegate instance. Used to complete
  // authentication process.
  SigninScreenHandlerDelegate* delegate_;

  base::WeakPtrFactory<ActiveDirectoryPasswordChangeScreenHandler>
      weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ActiveDirectoryPasswordChangeScreenHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_UI_WEBUI_CHROMEOS_LOGIN_ACTIVE_DIRECTORY_PASSWORD_CHANGE_SCREEN_HANDLER_H_
