// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/login_display_host_views.h"

#include <string>
#include <utility>

#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/chromeos/login/existing_user_controller.h"
#include "chrome/browser/chromeos/login/screens/gaia_view.h"
#include "chrome/browser/chromeos/login/ui/gaia_dialog_delegate.h"
#include "chrome/browser/chromeos/login/ui/login_display.h"
#include "chrome/browser/chromeos/login/ui/login_display_views.h"
#include "chrome/browser/chromeos/login/wizard_controller.h"
#include "chrome/browser/ui/webui/chromeos/login/signin_screen_handler.h"
#include "chromeos/login/auth/user_context.h"
#include "components/user_manager/user_names.h"

namespace chromeos {

namespace {

void ScheduleCompletionCallbacks(std::vector<base::OnceClosure>&& callbacks) {
  for (auto& callback : callbacks) {
    if (callback.is_null())
      continue;
    base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE,
                                                  std::move(callback));
  }
}

}  // namespace

LoginDisplayHostViews::LoginDisplayHostViews() : weak_factory_(this) {}

LoginDisplayHostViews::~LoginDisplayHostViews() {
  LoginScreenClient::Get()->SetDelegate(nullptr);
  ScheduleCompletionCallbacks(std::move(completion_callbacks_));
}

LoginDisplay* LoginDisplayHostViews::CreateLoginDisplay(
    LoginDisplay::Delegate* delegate) {
  return new LoginDisplayViews(delegate, this);
}

gfx::NativeWindow LoginDisplayHostViews::GetNativeWindow() const {
  NOTIMPLEMENTED();
  return nullptr;
}

OobeUI* LoginDisplayHostViews::GetOobeUI() const {
  if (!dialog_)
    return nullptr;
  return dialog_->GetOobeUI();
}

WebUILoginView* LoginDisplayHostViews::GetWebUILoginView() const {
  NOTREACHED();
  return nullptr;
}

void LoginDisplayHostViews::Finalize(base::OnceClosure completion_callback) {
  if (dialog_)
    dialog_->Close();

  completion_callbacks_.push_back(std::move(completion_callback));
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

void LoginDisplayHostViews::SetStatusAreaVisible(bool visible) {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::StartWizard(OobeScreen first_screen) {
  if (!GetOobeUI())
    return;

  wizard_controller_.reset(new WizardController(this, GetOobeUI()));
  wizard_controller_->Init(first_screen);
}

WizardController* LoginDisplayHostViews::GetWizardController() {
  NOTIMPLEMENTED();
  return nullptr;
}

void LoginDisplayHostViews::StartUserAdding(
    base::OnceClosure completion_callback) {
  completion_callbacks_.push_back(std::move(completion_callback));
}

void LoginDisplayHostViews::CancelUserAdding() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::OnStartSignInScreen(
    const LoginScreenContext& context) {
  // This function may be called early in startup flow, before LoginScreenClient
  // has been initialized. Wait until LoginScreenClient is initialized as it is
  // a common dependency.
  if (!LoginScreenClient::HasInstance()) {
    // TODO(jdufault): Add a timeout here / make sure we do not post infinitely.
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&LoginDisplayHostViews::OnStartSignInScreen,
                                  weak_factory_.GetWeakPtr(), context));
    return;
  }

  // There can only be one |ExistingUserController| instance at a time.
  existing_user_controller_.reset();
  existing_user_controller_ = std::make_unique<ExistingUserController>(this);

  // We need auth attempt results to notify views-based lock screen.
  existing_user_controller_->set_login_status_consumer(this);

  // Load the UI.
  existing_user_controller_->Init(user_manager::UserManager::Get()->GetUsers());
}

void LoginDisplayHostViews::OnPreferencesChanged() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::OnStartAppLaunch() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::OnStartArcKiosk() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::OnBrowserCreated() {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::StartVoiceInteractionOobe() {
  NOTIMPLEMENTED();
}

bool LoginDisplayHostViews::IsVoiceInteractionOobe() {
  NOTIMPLEMENTED();
  return false;
}

void LoginDisplayHostViews::UpdateGaiaDialogVisibility(bool visible) {
  if (visible == !!dialog_)
    return;

  if (visible) {
    dialog_ = new GaiaDialogDelegate(weak_factory_.GetWeakPtr());
    dialog_->Show();
    return;
  }

  if (users_.empty() && GetOobeUI()) {
    // Dialog could not be closed if there's no user in the login screen,
    // refreshing the dialog instead.
    GetOobeUI()->GetGaiaScreenView()->ShowGaiaAsync();
    return;
  }

  dialog_->Close();
}

void LoginDisplayHostViews::UpdateGaiaDialogSize(int width, int height) {
  if (dialog_)
    dialog_->SetSize(width, height);
}

const user_manager::UserList LoginDisplayHostViews::GetUsers() {
  return users_;
}

void LoginDisplayHostViews::HandleAuthenticateUser(
    const AccountId& account_id,
    const std::string& hashed_password,
    const password_manager::SyncPasswordData& sync_password_data,
    bool authenticated_by_pin,
    AuthenticateUserCallback callback) {
  DCHECK(!authenticated_by_pin);
  DCHECK_EQ(account_id.GetUserEmail(),
            gaia::SanitizeEmail(account_id.GetUserEmail()));

  on_authenticated_ = std::move(callback);

  UserContext user_context(account_id);
  user_context.SetKey(Key(chromeos::Key::KEY_TYPE_SALTED_SHA256_TOP_HALF,
                          std::string(), hashed_password));
  user_context.SetSyncPasswordData(sync_password_data);
  if (account_id.GetAccountType() == AccountType::ACTIVE_DIRECTORY)
    user_context.SetUserType(user_manager::USER_TYPE_ACTIVE_DIRECTORY);

  existing_user_controller_->Login(user_context, chromeos::SigninSpecifics());
}

void LoginDisplayHostViews::HandleAttemptUnlock(const AccountId& account_id) {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::HandleHardlockPod(const AccountId& account_id) {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::HandleRecordClickOnLockIcon(
    const AccountId& account_id) {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::HandleOnFocusPod(const AccountId& account_id) {
  NOTIMPLEMENTED();
}

void LoginDisplayHostViews::HandleOnNoPodFocused() {
  NOTIMPLEMENTED();
}

bool LoginDisplayHostViews::HandleFocusLockScreenApps(bool reverse) {
  NOTIMPLEMENTED();
  return false;
}

void LoginDisplayHostViews::HandleLoginAsGuest() {
  existing_user_controller_->Login(UserContext(user_manager::USER_TYPE_GUEST,
                                               user_manager::GuestAccountId()),
                                   chromeos::SigninSpecifics());
}

void LoginDisplayHostViews::OnAuthFailure(const AuthFailure& error) {
  if (on_authenticated_)
    std::move(on_authenticated_).Run(false);
}

void LoginDisplayHostViews::OnAuthSuccess(const UserContext& user_context) {
  if (on_authenticated_)
    std::move(on_authenticated_).Run(true);
}

void LoginDisplayHostViews::OnDialogDestroyed(
    const GaiaDialogDelegate* dialog) {
  if (dialog == dialog_) {
    dialog_ = nullptr;
    wizard_controller_.reset();
  }
}

void LoginDisplayHostViews::SetUsers(const user_manager::UserList& users) {
  users_ = users;
}

}  // namespace chromeos
