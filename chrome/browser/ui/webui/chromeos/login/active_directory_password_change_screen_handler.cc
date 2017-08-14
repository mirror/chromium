// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/active_directory_password_change_screen_handler.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/chromeos/login/oobe_screen.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "chrome/grit/generated_resources.h"
#include "chromeos/login/auth/authpolicy_login_helper.h"
#include "chromeos/login/auth/key.h"
#include "components/login/localized_values_builder.h"
#include "components/user_manager/known_user.h"

namespace chromeos {

namespace {

constexpr char kJsScreenPath[] = "login.ActiveDirectoryPasswordChangeScreen";
constexpr char kUsernameKey[] = "username";
constexpr char kErrorKey[] = "error";

// Possible error states of the Active Directory password change screen. Must be
// in the same order as ACTIVE_DIRECTORY_PASSWORD_CHANGE_ERROR_STATE enum
// values.
enum class ActiveDirectoryPasswordChangeErrorState {
  WRONG_OLD_PASSWORD = 0,
  NEW_PASSWORD_REJECTED = 1,
};

}  // namespace

ActiveDirectoryPasswordChangeScreenHandler::
    ActiveDirectoryPasswordChangeScreenHandler()
    : BaseScreenHandler(OobeScreen::SCREEN_ACTIVE_DIRECTORY_PASSWORD_CHANGE),
      authpolicy_login_helper_(base::MakeUnique<AuthPolicyLoginHelper>()),
      weak_factory_(this) {
  set_call_js_prefix(kJsScreenPath);
}

ActiveDirectoryPasswordChangeScreenHandler::
    ~ActiveDirectoryPasswordChangeScreenHandler() {}

void ActiveDirectoryPasswordChangeScreenHandler::DeclareLocalizedValues(
    ::login::LocalizedValuesBuilder* builder) {
  builder->Add("adPassChangeOldPasswordHint",
               IDS_AD_PASSWORD_CHANGE_OLD_PASSWORD_HINT);
  builder->Add("adPassChangeNewPasswordHint",
               IDS_AD_PASSWORD_CHANGE_NEW_PASSWORD_HINT);
  builder->Add("adPassChangeRepeatNewPasswordHint",
               IDS_AD_PASSWORD_CHANGE_REPEAT_NEW_PASSWORD_HINT);
  builder->Add("adPassChangeMessage", IDS_AD_PASSWORD_CHANGE_MESSAGE);
  builder->Add("adPassChangeOldPasswordError",
               IDS_AD_PASSWORD_CHANGE_INVALID_PASSWORD_ERROR);
  builder->Add("adPassChangeNewPasswordRejected",
               IDS_AD_PASSWORD_CHANGE_NEW_PASSWORD_REJECTED_ERROR);
  builder->Add("adPassChangePasswordsMismatch",
               IDS_AD_PASSWORD_CHANGE_PASSWORDS_MISMATCH_ERROR);
}

void ActiveDirectoryPasswordChangeScreenHandler::Initialize() {}

void ActiveDirectoryPasswordChangeScreenHandler::RegisterMessages() {
  AddCallback("completePasswordChange",
              &ActiveDirectoryPasswordChangeScreenHandler::
                  HandleCompletePasswordChange);
  AddCallback("cancel",
              &ActiveDirectoryPasswordChangeScreenHandler::HandleCancel);
}

void ActiveDirectoryPasswordChangeScreenHandler::HandleCompletePasswordChange(
    const std::string& username,
    const std::string& old_password,
    const std::string& new_password) {
  authpolicy_login_helper_->AuthenticateUser(
      username, std::string() /* object_guid */,
      old_password + "\n" + new_password + "\n" + new_password,
      base::BindOnce(
          &ActiveDirectoryPasswordChangeScreenHandler::OnAuthFinished,
          weak_factory_.GetWeakPtr(), username, Key(new_password)));
}

void ActiveDirectoryPasswordChangeScreenHandler::HandleCancel() {
  authpolicy_login_helper_->CancelRequestsAndRestart();
}

void ActiveDirectoryPasswordChangeScreenHandler::ShowScreen(
    const std::string& username,
    SigninScreenHandlerDelegate* delegate) {
  delegate_ = delegate;
  base::DictionaryValue data;
  data.SetString(kUsernameKey, username);
  ShowScreenWithData(OobeScreen::SCREEN_ACTIVE_DIRECTORY_PASSWORD_CHANGE,
                     &data);
}

void ActiveDirectoryPasswordChangeScreenHandler::ShowScreenWithError(
    int error) {
  base::DictionaryValue data;
  data.SetInteger(kErrorKey, error);
  ShowScreenWithData(OobeScreen::SCREEN_ACTIVE_DIRECTORY_PASSWORD_CHANGE,
                     &data);
}

void ActiveDirectoryPasswordChangeScreenHandler::OnAuthFinished(
    const std::string& username,
    const Key& key,
    authpolicy::ErrorType error,
    const authpolicy::ActiveDirectoryAccountInfo& account_info) {
  switch (error) {
    case authpolicy::ERROR_NONE: {
      DCHECK(account_info.has_account_id() &&
             !account_info.account_id().empty());
      const AccountId account_id = user_manager::known_user::GetAccountId(
          username, account_info.account_id(), AccountType::ACTIVE_DIRECTORY);
      DCHECK(delegate_);
      delegate_->SetDisplayAndGivenName(account_info.display_name(),
                                        account_info.given_name());
      UserContext user_context(account_id);
      user_context.SetKey(key);
      user_context.SetAuthFlow(UserContext::AUTH_FLOW_ACTIVE_DIRECTORY);
      user_context.SetIsUsingOAuth(false);
      user_context.SetUserType(
          user_manager::UserType::USER_TYPE_ACTIVE_DIRECTORY);
      delegate_->CompleteLogin(user_context);
      break;
    }
    case authpolicy::ERROR_BAD_PASSWORD:
      ShowScreenWithError(static_cast<int>(
          ActiveDirectoryPasswordChangeErrorState::WRONG_OLD_PASSWORD));
      break;
    case authpolicy::ERROR_PASSWORD_REJECTED:
      ShowScreenWithError(static_cast<int>(
          ActiveDirectoryPasswordChangeErrorState::NEW_PASSWORD_REJECTED));
      break;
    default:
      NOTREACHED() << "Unhandled error: " << error;
      ShowScreen(username, delegate_);
  }
}

}  // namespace chromeos
