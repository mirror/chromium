// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/login_display_host_views.h"

#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/screens/user_selection_screen.h"
#include "chrome/browser/chromeos/settings/cros_settings.h"
#include "chrome/browser/ui/ash/login_screen_client.h"
#include "chrome/browser/ui/ash/system_tray_client.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "components/user_manager/known_user.h"

namespace chromeos {

namespace {

// TODO(jdufault): Deduplicate this and
// user_selection_screen::GetOwnerAccountId().
AccountId GetOwnerAccountId() {
  std::string owner_email;
  chromeos::CrosSettings::Get()->GetString(chromeos::kDeviceOwner,
                                           &owner_email);
  const AccountId owner = user_manager::known_user::GetAccountId(
      owner_email, std::string() /* id */, AccountType::UNKNOWN);
  return owner;
};

}  // namespace

LoginDisplayHostViews::LoginDisplayHostViews() : weak_factory_(this) {}

LoginDisplayHostViews::~LoginDisplayHostViews() = default;

LoginDisplay* LoginDisplayHostViews::CreateLoginDisplay(
    LoginDisplay::Delegate* delegate) {
  NOTREACHED();
  return nullptr;
}

gfx::NativeWindow LoginDisplayHostViews::GetNativeWindow() const {
  NOTREACHED();
  return nullptr;
}

OobeUI* LoginDisplayHostViews::GetOobeUI() const {
  NOTREACHED();
  return nullptr;
}

WebUILoginView* LoginDisplayHostViews::GetWebUILoginView() const {
  NOTREACHED();
  return nullptr;
}

void LoginDisplayHostViews::BeforeSessionStart() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::Finalize(base::OnceClosure completion_callback) {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::OpenInternetDetailDialog(
    const std::string& network_id) {
  NOTREACHED();
}

void LoginDisplayHostViews::SetStatusAreaVisible(bool visible) {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::StartWizard(OobeScreen first_screen) {
  NOTIMPLEMENTED();
}

WizardController* LoginDisplayHostViews::GetWizardController() {
  NOTIMPLEMENTED();
  return nullptr;
}

AppLaunchController* LoginDisplayHostViews::GetAppLaunchController() {
  NOTIMPLEMENTED();
  return nullptr;
}

void LoginDisplayHostViews::StartUserAdding(
    base::OnceClosure completion_callback) {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::CancelUserAdding() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::OnStartSignInScreen(
    const LoginScreenContext& context,
    const user_manager::UserList& all_users) {
  // This function may be called early in startup flow, before LoginScreenClient
  // has been initialized. Wait until LoginScreenClient is initialized.
  if (!LoginScreenClient::HasInstance()) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&LoginDisplayHostViews::OnStartSignInScreen,
                       weak_factory_.GetWeakPtr(), context, all_users));
    return;
  }

  // Filter users.
  bool show_users_on_signin;
  CrosSettings::Get()->GetBoolean(kAccountsPrefShowUserNamesOnSignIn,
                                  &show_users_on_signin);
  const user_manager::UserList filtered_users =
      chromeos::ExistingUserController::FilterUsersForLogin(
          all_users, show_users_on_signin /*show_users_on_signin*/);

  const AccountId owner_account = GetOwnerAccountId();

  // Convert |filtered_users| to mojo structures.
  std::vector<ash::mojom::LoginUserInfoPtr> users_list;
  for (user_manager::User* user : filtered_users) {
    auto mojo_user = ash::mojom::LoginUserInfo::New();

    bool is_owner = user->GetAccountId() == owner_account;

    const bool is_public_account =
        user->GetType() == user_manager::USER_TYPE_PUBLIC_ACCOUNT;
    const proximity_auth::mojom::AuthType initial_auth_type =
        is_public_account
            ? proximity_auth::mojom::AuthType::EXPAND_THEN_USER_CLICK
            : (chromeos::UserSelectionScreen::ShouldForceOnlineSignIn(user)
                   ? proximity_auth::mojom::AuthType::ONLINE_SIGN_IN
                   : proximity_auth::mojom::AuthType::OFFLINE_PASSWORD);

    chromeos::UserSelectionScreen::FillUserMojoStruct(
        user, is_owner, false /*is_signin_to_add*/, initial_auth_type,
        mojo_user.get());

    users_list.push_back(std::move(mojo_user));
  }

  bool show_guest = user_manager::UserManager::Get()->IsGuestSessionAllowed();

  // Load the login screen.
  LoginScreenClient::Get()->ShowLoginScreen();
  LoginScreenClient::Get()->LoadUsers(std::move(users_list), show_guest);
}

void LoginDisplayHostViews::OnPreferencesChanged() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::StartAppLaunch(const std::string& app_id,
                                           bool diagnostic_mode,
                                           bool is_auto_launch) {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::StartDemoAppLaunch() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::StartArcKiosk(const AccountId& account_id) {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::StartVoiceInteractionOobe() {
  NOTIMPLEMENTED();
}

bool LoginDisplayHostViews::IsVoiceInteractionOobe() {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace chromeos
