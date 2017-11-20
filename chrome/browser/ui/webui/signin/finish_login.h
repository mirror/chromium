// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SIGNIN_FINISH_LOGIN_H_
#define CHROME_BROWSER_UI_WEBUI_SIGNIN_FINISH_LOGIN_H_

#include <string>

#include "base/files/file_path.h"
#include "chrome/browser/ui/sync/one_click_signin_sync_starter.h"
#include "chrome/browser/ui/webui/signin/signin_email_confirmation_dialog.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "google_apis/gaia/gaia_auth_consumer.h"
#include "components/signin/core/browser/account_info.h"

class InlineLoginHandlerImpl;

namespace content {
class WebContents;
}

namespace net {
class URLRequestContextGetter;
}

class InlineSigninHelperDelegate {
 public:
  InlineSigninHelperDelegate();
  virtual ~InlineSigninHelperDelegate();

  virtual void CloseTab(bool show_account_management) = 0;
  virtual void CloseLoginDialog() = 0;
  virtual void HandleLoginError(const std::string& error_msg,
                                const base::string16& email) = 0;
  virtual void SyncStarterCallback(
      OneClickSigninSyncStarter::SyncSetupResult result) = 0;

  base::WeakPtr<InlineSigninHelperDelegate> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<InlineSigninHelperDelegate> weak_factory_;
};

// This struct exists to pass paramters to the FinishCompleteLogin() method,
// since the base::Bind() call does not support this many template args.
struct FinishCompleteLoginParams {
 public:
  FinishCompleteLoginParams(InlineSigninHelperDelegate* delegate,
                            net::URLRequestContextGetter* context_getter,
                            signin_metrics::AccessPoint signin_access_point,
                            signin_metrics::Reason signin_reason,
                            const base::FilePath& profile_path,
                            bool confirm_untrusted_signin,
                            const std::string& email,
                            const std::string& gaia_id,
                            const std::string& password,
                            const std::string& session_index,
                            const std::string& auth_code,
                            bool choose_what_to_sync,
                            bool is_force_sign_in_with_usermanager,
                            bool is_auto_closable,
                            bool should_show_account_management);
  FinishCompleteLoginParams(const FinishCompleteLoginParams& other);
  ~FinishCompleteLoginParams();

  // Pointer to WebUI handler.  May be nullptr.
  InlineSigninHelperDelegate* delegate;
  // URL context getter that contains the sign in cookies.
  net::URLRequestContextGetter* context_getter;
  signin_metrics::AccessPoint signin_access_point;
  signin_metrics::Reason signin_reason;
  // Path to profile being signed in. Non empty only when unlocking a profile
  // from the user manager.
  base::FilePath profile_path;
  // When true, an extra prompt will be shown to the user before sign in
  // completes.
  bool confirm_untrusted_signin;
  // Email address of the account used to sign in.
  std::string email;
  // Obfustcated gaia id of the account used to sign in.
  std::string gaia_id;
  // Password of the account used to sign in.
  std::string password;
  // Index within gaia cookie of the account used to sign in.  Used only
  // with password combined signin flow.
  std::string session_index;
  // Authentication code used to exchange for a login scoped refresh token
  // for the account used to sign in.  Used only with password separated
  // signin flow.
  std::string auth_code;
  // True if the user wants to configure sync before signing in.
  bool choose_what_to_sync;
  // True if user signing in with UserManager when force-sign-in policy is
  // enabled.
  bool is_force_sign_in_with_usermanager;
  bool is_auto_closable;
  bool should_show_account_management;
};

void FinishLogin(const FinishCompleteLoginParams& params,
                 Profile* profile,
                 Profile::CreateStatus status);

// Handles details of signing the user in with SigninManager and turning on
// sync after InlineLoginHandlerImpl has acquired the auth tokens from GAIA.
// This is a separate class from InlineLoginHandlerImpl because the full signin
// process is asynchronous and can outlive the signin UI.
// InlineLoginHandlerImpl is destryed once the UI is closed.
class InlineSigninHelper : public GaiaAuthConsumer {
 public:
  InlineSigninHelper(base::WeakPtr<InlineSigninHelperDelegate> delegate,
                     net::URLRequestContextGetter* context_getter,
                     Profile* profile,
                     Profile::CreateStatus create_status,
                     signin_metrics::AccessPoint signin_access_point,
                     signin_metrics::Reason signin_reason,
                     const std::string& email,
                     const std::string& gaia_id,
                     const std::string& password,
                     const std::string& session_index,
                     const std::string& auth_code,
                     const std::string& signin_scoped_device_id,
                     bool choose_what_to_sync,
                     bool confirm_untrusted_signin,
                     bool is_force_sign_in_with_usermanager,
                     bool is_auto_closable,
                     bool should_show_account_management);
  ~InlineSigninHelper() override;

 private:
  // Handles cross account sign in error. If the supplied |email| does not match
  // the last signed in email of the current profile, then Chrome will show a
  // confirmation dialog before starting sync. It returns true if there is a
  // cross account error, and false otherwise.
  bool HandleCrossAccountError(
      const std::string& refresh_token,
      OneClickSigninSyncStarter::ConfirmationRequired confirmation_required,
      OneClickSigninSyncStarter::StartSyncMode start_mode);

  // Callback used with ConfirmEmailDialogDelegate.
  void ConfirmEmailAction(
      content::WebContents* web_contents,
      const std::string& refresh_token,
      OneClickSigninSyncStarter::ConfirmationRequired confirmation_required,
      OneClickSigninSyncStarter::StartSyncMode start_mode,
      SigninEmailConfirmationDialog::Action action);

  // Overridden from GaiaAuthConsumer.
  void OnClientOAuthSuccess(const ClientOAuthResult& result) override;
  void OnClientOAuthFailure(const GoogleServiceAuthError& error) override;

  void OnClientOAuthSuccessAndBrowserOpened(const ClientOAuthResult& result,
                                            Profile* profile,
                                            Profile::CreateStatus status);

  // Creates the sync starter.  Virtual for tests. Call to exchange oauth code
  // for tokens.
  virtual void CreateSyncStarter(
      Browser* browser,
      signin_metrics::AccessPoint signin_access_point,
      signin_metrics::Reason signin_reason,
      const std::string& refresh_token,
      OneClickSigninSyncStarter::ProfileMode profile_mode,
      OneClickSigninSyncStarter::StartSyncMode start_mode,
      OneClickSigninSyncStarter::ConfirmationRequired confirmation_required);

  GaiaAuthFetcher gaia_auth_fetcher_;
  base::WeakPtr<InlineSigninHelperDelegate> delegate_;
  Profile* profile_;
  Profile::CreateStatus create_status_;
  signin_metrics::AccessPoint signin_access_point_;
  signin_metrics::Reason signin_reason_;
  std::string email_;
  std::string gaia_id_;
  std::string password_;
  std::string session_index_;
  std::string auth_code_;
  bool choose_what_to_sync_;
  bool confirm_untrusted_signin_;
  bool is_force_sign_in_with_usermanager_;
  bool is_auto_closable_;
  bool should_show_account_management_;

  DISALLOW_COPY_AND_ASSIGN(InlineSigninHelper);
};

class DiceTurnSyncOnHelper {
public:
  DiceTurnSyncOnHelper(
    base::WeakPtr<InlineSigninHelperDelegate> delegate,
    Profile* profile,
    Browser* browser,
    signin_metrics::AccessPoint signin_access_point,
    signin_metrics::Reason signin_reason,
    const std::string& account_id);
  ~DiceTurnSyncOnHelper() override;

private:
  // Handles cross account sign in error. If the supplied |email| does not match
  // the last signed in email of the current profile, then Chrome will show a
  // confirmation dialog before starting sync. It returns true if there is a
  // cross account error, and false otherwise.
  bool HandleCrossAccountError();

  // Callback used with ConfirmEmailDialogDelegate.
  void ConfirmEmailAction(
    content::WebContents* web_contents,
    SigninEmailConfirmationDialog::Action action);

  bool CanTurnSyncOn();

  void StartTurnOnSync();

  // Creates the sync starter.  Virtual for tests.
  virtual void CreateSyncStarter(
    OneClickSigninSyncStarter::ProfileMode profile_mode);

  base::WeakPtr<InlineSigninHelperDelegate> delegate_;
  Profile* profile_;
  Browser* browser_;
  signin_metrics::AccessPoint signin_access_point_;
  signin_metrics::Reason signin_reason_;
  const std::string account_id_;
  const AccountInfo account_info_;

  DISALLOW_COPY_AND_ASSIGN(DiceTurnSyncOnHelper);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SIGNIN_FINISH_LOGIN_H_
