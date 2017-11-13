// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/login_display_host_views.h"

#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chromeos/login/screens/user_selection_screen.h"
#include "chrome/browser/ui/ash/login_screen_client.h"
#include "chrome/browser/ui/ash/system_tray_client.h"

namespace chromeos {

LoginDisplayHostViews::LoginDisplayHostViews() {}

LoginDisplayHostViews::~LoginDisplayHostViews() {}

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
    const user_manager::UserList& users) {
  // |StartSignInScreen| will be called during chrome initialization, which
  // means that mojo APIs may not have been set up yet. Wait until there is a
  // LockScreenClient instance.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(
                     [](const user_manager::UserList& users) {
                       LoginScreenClient::Get()->ShowLoginScreen();

                       #if false  // FIXME/DONOTSUBMIT
                       std::vector<ash::mojom::LoginUserInfoPtr> users_list;
                       for (user_manager::User* user : users) {
                         auto mojo_user = ash::mojom::LoginUserInfo::New();

                         // TODO(crbug.com/784495): Properly plumb constants to
                         // this function.
                         chromeos::UserSelectionScreen::FillUserMojoStruct(
                             user, false /*is_owner*/,
                             false /*is_signin_to_add*/,
                             proximity_auth::mojom::AuthType::ONLINE_SIGN_IN,
                             mojo_user.get());
                       }
                       // TODO(crbug.com/784495): Properly plumb show_guest.
                       LoginScreenClient::Get()->LoadUsers(
                           std::move(users_list), false /*show_guest*/);
                        #endif
                     },
                     users));
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
