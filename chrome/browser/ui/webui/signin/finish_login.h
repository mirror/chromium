// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SIGNIN_FINISH_LOGIN_H_
#define CHROME_BROWSER_UI_WEBUI_SIGNIN_FINISH_LOGIN_H_

#include <string>

#include "base/files/file_path.h"
#include "chrome/browser/ui/sync/one_click_signin_sync_starter.h"
#include "chrome/browser/ui/webui/signin/signin_email_confirmation_dialog.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "google_apis/gaia/gaia_auth_consumer.h"

class InlineSigninHelperDelegate {
 public:
  InlineSigninHelperDelegate();
  virtual ~InlineSigninHelperDelegate();

  virtual void SyncStarterCallback(
      OneClickSigninSyncStarter::SyncSetupResult result) = 0;

  base::WeakPtr<InlineSigninHelperDelegate> GetWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  base::WeakPtrFactory<InlineSigninHelperDelegate> weak_factory_;
};

// Handles details of signing the user in with SigninManager and turning on
// sync for an account that is already present in the token service.
class DiceTurnSyncOnHelper {
 public:
  DiceTurnSyncOnHelper(base::WeakPtr<InlineSigninHelperDelegate> delegate,
                       Profile* profile,
                       Browser* browser,
                       signin_metrics::AccessPoint signin_access_point,
                       signin_metrics::Reason signin_reason,
                       const std::string& account_id);
  virtual ~DiceTurnSyncOnHelper();

 private:
  void StartTurnOnSync();

  bool HandleCanOfferSigninError();

  // Handles cross account sign in error. If the supplied |email| does not match
  // the last signed in email of the current profile, then Chrome will show a
  // confirmation dialog before starting sync. It returns true if there is a
  // cross account error, and false otherwise.
  bool HandleCrossAccountError();

  // Callback used with ConfirmEmailDialogDelegate.
  void ConfirmEmailAction(content::WebContents* web_contents,
                          SigninEmailConfirmationDialog::Action action);

  // Creates the sync starter.  Virtual for tests.
  virtual void CreateSyncStarter(
      OneClickSigninSyncStarter::ProfileMode profile_mode);

  base::WeakPtr<InlineSigninHelperDelegate> delegate_;
  Profile* profile_;
  Browser* browser_;
  signin_metrics::AccessPoint signin_access_point_;
  signin_metrics::Reason signin_reason_;
  const AccountInfo account_info_;

  DISALLOW_COPY_AND_ASSIGN(DiceTurnSyncOnHelper);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SIGNIN_FINISH_LOGIN_H_
