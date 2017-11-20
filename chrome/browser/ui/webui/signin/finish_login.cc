// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/finish_login.h"

#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/signin/about_signin_internals_factory.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/chrome_signin_client_factory.h"
#include "chrome/browser/signin/investigator_dependency_provider.h"
#include "chrome/browser/signin/local_auth.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_error_controller_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/signin/signin_promo.h"
#include "chrome/browser/signin/signin_util.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/user_manager.h"
#include "chrome/browser/ui/webui/signin/inline_login_handler_impl.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/about_signin_internals.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_error_controller.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "content/public/browser/storage_partition.h"
#include "google_apis/gaia/gaia_constants.h"
#include "net/base/url_util.h"
#include "ui/base/l10n/l10n_util.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/webui/signin/login_ui_service.h"
#include "chrome/browser/ui/webui/signin/login_ui_service_factory.h"

namespace {

AccountInfo GetAccountInfo(Profile* profile, const std::string& account_id) {
  return AccountTrackerServiceFactory::GetForProfile(profile)->GetAccountInfo(
      account_id);
}

bool IsCrossAccountError(Profile* profile,
                         const std::string& email,
                         const std::string& gaia_id) {
  InvestigatorDependencyProvider provider(profile);
  InvestigatedScenario scenario =
      SigninInvestigator(email, gaia_id, &provider).Investigate();
  return scenario == InvestigatedScenario::DIFFERENT_ACCOUNT;
}

}  // namespace

InlineSigninHelperDelegate::InlineSigninHelperDelegate()
    : weak_factory_(this){};

InlineSigninHelperDelegate::~InlineSigninHelperDelegate() = default;

// -----------------------------------------------------------------------------
DiceTurnSyncOnHelper::DiceTurnSyncOnHelper(
    base::WeakPtr<InlineSigninHelperDelegate> delegate,
    Profile* profile,
    Browser* browser,
    signin_metrics::AccessPoint signin_access_point,
    signin_metrics::Reason signin_reason,
    const std::string& account_id)
    : delegate_(delegate),
      profile_(profile),
      browser_(browser),
      signin_access_point_(signin_access_point),
      signin_reason_(signin_reason),
      account_info_(GetAccountInfo(profile, account_id)) {
  DCHECK(profile_);
  DCHECK(browser_);
  DCHECK(!account_info_.account_id.empty());
  DCHECK(!account_info_.email.empty());
  DCHECK(!account_info_.gaia.empty());

  // Should not start synching if the profile is already authenticated
  DCHECK(SigninManagerFactory::GetForProfile(profile_)->IsAuthenticated());

  // Force sign-in uses the modal sign-in flow.
  DCHECK(!signin_util::IsForceSigninEnabled());

  DCHECK_EQ(signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT,
            signin_reason_);

  StartTurnOnSync();
}

DiceTurnSyncOnHelper::~DiceTurnSyncOnHelper() {}

void DiceTurnSyncOnHelper::StartTurnOnSync() {
  Browser* browser = chrome::FindLastActiveWithProfile(profile_);
  SigninManager* signin_manager = SigninManagerFactory::GetForProfile(profile_);
  std::string primary_email =
      signin_manager->GetAuthenticatedAccountInfo().email;

  if (!HandleCanOfferSigninError() && !HandleCrossAccountError()) {
    CreateSyncStarter(OneClickSigninSyncStarter::CURRENT_PROFILE);
    base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
  }
}

bool DiceTurnSyncOnHelper::HandleCanOfferSigninError() {
  std::string error_msg;
  bool can_offer = InlineLoginHandlerImpl::CanOffer(
      profile_, InlineLoginHandlerImpl::CAN_OFFER_FOR_ALL, account_info_.gaia,
      account_info_.email, &error_msg);
  if (can_offer)
    return true;

  // Display the error message
  LoginUIServiceFactory::GetForProfile(profile_)->DisplayLoginResult(
      browser_, base::UTF8ToUTF16(error_msg),
      base::UTF8ToUTF16(account_info_.email));

  // Kill self a bit later
  delegate_->SyncStarterCallback(OneClickSigninSyncStarter::SYNC_SETUP_FAILURE);
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
  return false;
}

bool DiceTurnSyncOnHelper::HandleCrossAccountError() {
  std::string last_email =
      profile_->GetPrefs()->GetString(prefs::kGoogleServicesLastUsername);

  // TODO(skym): Warn for high risk upgrade scenario, crbug.com/572754.
  if (!IsCrossAccountError(profile_, account_info_.email, account_info_.gaia))
    return false;

  Browser* browser = chrome::FindLastActiveWithProfile(profile_);
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();

  SigninEmailConfirmationDialog::AskForConfirmation(
      web_contents, profile_, last_email, account_info_.email,
      base::Bind(&DiceTurnSyncOnHelper::ConfirmEmailAction,
                 base::Unretained(this), web_contents));
  return true;
}

void DiceTurnSyncOnHelper::ConfirmEmailAction(
    content::WebContents* web_contents,
    SigninEmailConfirmationDialog::Action action) {
  Browser* browser = chrome::FindLastActiveWithProfile(profile_);
  switch (action) {
    case SigninEmailConfirmationDialog::CREATE_NEW_USER:
      base::RecordAction(
          base::UserMetricsAction("Signin_ImportDataPrompt_DontImport"));
      CreateSyncStarter(OneClickSigninSyncStarter::NEW_PROFILE);
      break;
    case SigninEmailConfirmationDialog::START_SYNC:
      base::RecordAction(
          base::UserMetricsAction("Signin_ImportDataPrompt_ImportData"));
      CreateSyncStarter(OneClickSigninSyncStarter::CURRENT_PROFILE);
      break;
    case SigninEmailConfirmationDialog::CLOSE:
      base::RecordAction(
          base::UserMetricsAction("Signin_ImportDataPrompt_Cancel"));
      if (delegate_) {
        delegate_->SyncStarterCallback(
            OneClickSigninSyncStarter::SYNC_SETUP_FAILURE);
      }
      break;
    default:
      DCHECK(false) << "Invalid action";
  }
  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}

void DiceTurnSyncOnHelper::CreateSyncStarter(
    OneClickSigninSyncStarter::ProfileMode profile_mode) {
  // OneClickSigninSyncStarter will delete itself once the job is done.
  new OneClickSigninSyncStarter(
      profile_, browser_, account_info_.account_id, signin_access_point_,
      signin_reason_, profile_mode,
      base::Bind(&InlineSigninHelperDelegate::SyncStarterCallback, delegate_));
}
