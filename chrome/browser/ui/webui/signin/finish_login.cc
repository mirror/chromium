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

namespace {

void LogHistogramValue(signin_metrics::AccessPointAction action) {
  UMA_HISTOGRAM_ENUMERATION("Signin.AllAccessPointActions", action,
                            signin_metrics::HISTOGRAM_MAX);
}

void UnlockProfileAndHideLoginUI(const base::FilePath profile_path,
                                 InlineSigninHelperDelegate* delegate) {
  if (!profile_path.empty()) {
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    if (profile_manager) {
      ProfileAttributesEntry* entry;
      if (profile_manager->GetProfileAttributesStorage()
              .GetProfileAttributesWithPath(profile_path, &entry)) {
        entry->SetIsSigninRequired(false);
      }
    }
  }
  if (delegate)
    delegate->CloseLoginDialog();

  UserManager::Hide();
}

bool IsCrossAccountError(Profile* profile,
                         const std::string& email,
                         const std::string& gaia_id) {
  InvestigatorDependencyProvider provider(profile);
  InvestigatedScenario scenario =
      SigninInvestigator(email, gaia_id, &provider).Investigate();

  return scenario == InvestigatedScenario::DIFFERENT_ACCOUNT;
}

// Argument to CanOffer().
enum CanOfferFor { CAN_OFFER_FOR_ALL, CAN_OFFER_FOR_SECONDARY_ACCOUNT };

bool CanOffer(Profile* profile,
              CanOfferFor can_offer_for,
              const std::string& gaia_id,
              const std::string& email,
              std::string* error_message) {
  if (error_message)
    error_message->clear();

  if (!profile)
    return false;

  SigninManager* manager = SigninManagerFactory::GetForProfile(profile);
  if (manager && !manager->IsSigninAllowed())
    return false;

  if (!ChromeSigninClient::ProfileAllowsSigninCookies(profile))
    return false;

  if (!email.empty()) {
    if (!manager)
      return false;

    // Make sure this username is not prohibited by policy.
    if (!manager->IsAllowedUsername(email)) {
      if (error_message) {
        error_message->assign(
            l10n_util::GetStringUTF8(IDS_SYNC_LOGIN_NAME_PROHIBITED));
      }
      return false;
    }

    if (can_offer_for == CAN_OFFER_FOR_SECONDARY_ACCOUNT)
      return true;

    // If the signin manager already has an authenticated name, then this is a
    // re-auth scenario.  Make sure the email just signed in corresponds to
    // the one sign in manager expects.
    std::string current_email = manager->GetAuthenticatedAccountInfo().email;
    const bool same_email = gaia::AreEmailsSame(current_email, email);
    if (!current_email.empty() && !same_email) {
      UMA_HISTOGRAM_ENUMERATION("Signin.Reauth",
                                signin_metrics::HISTOGRAM_ACCOUNT_MISSMATCH,
                                signin_metrics::HISTOGRAM_REAUTH_MAX);
      if (error_message) {
        error_message->assign(l10n_util::GetStringFUTF8(
            IDS_SYNC_WRONG_EMAIL, base::UTF8ToUTF16(current_email)));
      }
      return false;
    }

    // If some profile, not just the current one, is already connected to this
    // account, don't show the infobar.
    if (g_browser_process && !same_email) {
      ProfileManager* profile_manager = g_browser_process->profile_manager();
      if (profile_manager) {
        std::vector<ProfileAttributesEntry*> entries =
            profile_manager->GetProfileAttributesStorage()
                .GetAllProfilesAttributes();

        for (const ProfileAttributesEntry* entry : entries) {
          // For backward compatibility, need to check also the username of the
          // profile, since the GAIA ID may not have been set yet in the
          // ProfileAttributesStorage.  It will be set once the profile
          // is opened.
          std::string profile_gaia_id = entry->GetGAIAId();
          std::string profile_email = base::UTF16ToUTF8(entry->GetUserName());
          if (gaia_id == profile_gaia_id ||
              gaia::AreEmailsSame(email, profile_email)) {
            if (error_message) {
              error_message->assign(
                  l10n_util::GetStringUTF8(IDS_SYNC_USER_NAME_IN_USE_ERROR));
            }
            return false;
          }
        }
      }
    }

    // With force sign in enabled, cross account sign in is not allowed.
    if (signin_util::IsForceSigninEnabled() &&
        IsCrossAccountError(profile, email, gaia_id)) {
      if (error_message) {
        std::string last_email =
            profile->GetPrefs()->GetString(prefs::kGoogleServicesLastUsername);
        error_message->assign(l10n_util::GetStringFUTF8(
            IDS_SYNC_USED_PROFILE_ERROR, base::UTF8ToUTF16(last_email)));
      }
      return false;
    }
  }

  return true;
}

}  // namespace

InlineSigninHelperDelegate::InlineSigninHelperDelegate()
    : weak_factory_(this){};

InlineSigninHelperDelegate::~InlineSigninHelperDelegate() = default;

FinishCompleteLoginParams::FinishCompleteLoginParams(
    InlineSigninHelperDelegate* delegate,
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
    bool should_show_account_management)
    : delegate(delegate),
      context_getter(context_getter),
      signin_access_point(signin_access_point),
      signin_reason(signin_reason),
      profile_path(profile_path),
      confirm_untrusted_signin(confirm_untrusted_signin),
      email(email),
      gaia_id(gaia_id),
      password(password),
      session_index(session_index),
      auth_code(auth_code),
      choose_what_to_sync(choose_what_to_sync),
      is_force_sign_in_with_usermanager(is_force_sign_in_with_usermanager),
      is_auto_closable(is_auto_closable),
      should_show_account_management(should_show_account_management) {}

FinishCompleteLoginParams::FinishCompleteLoginParams(
    const FinishCompleteLoginParams& other) = default;

FinishCompleteLoginParams::~FinishCompleteLoginParams() = default;

void FinishLogin(const FinishCompleteLoginParams& params,
                 Profile* profile,
                 Profile::CreateStatus status) {
  LogHistogramValue(signin_metrics::HISTOGRAM_ACCEPTED);
  bool switch_to_advanced =
      params.choose_what_to_sync &&
      (params.signin_access_point !=
       signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS);
  LogHistogramValue(switch_to_advanced
                        ? signin_metrics::HISTOGRAM_WITH_ADVANCED
                        : signin_metrics::HISTOGRAM_WITH_DEFAULTS);

  CanOfferFor can_offer_for = CAN_OFFER_FOR_ALL;
  switch (params.signin_reason) {
    case signin_metrics::Reason::REASON_ADD_SECONDARY_ACCOUNT:
      can_offer_for = CAN_OFFER_FOR_SECONDARY_ACCOUNT;
      break;
    case signin_metrics::Reason::REASON_REAUTHENTICATION:
    case signin_metrics::Reason::REASON_UNLOCK: {
      std::string primary_username =
          SigninManagerFactory::GetForProfile(profile)
              ->GetAuthenticatedAccountInfo()
              .email;
      if (!gaia::AreEmailsSame(params.email, primary_username))
        can_offer_for = CAN_OFFER_FOR_SECONDARY_ACCOUNT;
      break;
    }
    default:
      // No need to change |can_offer_for|.
      break;
  }

  std::string error_msg;
  bool can_offer = CanOffer(profile, can_offer_for, params.gaia_id,
                            params.email, &error_msg);
  if (!can_offer) {
    if (params.delegate) {
      params.delegate->HandleLoginError(error_msg,
                                        base::UTF8ToUTF16(params.email));
    }
    return;
  }

  AboutSigninInternals* about_signin_internals =
      AboutSigninInternalsFactory::GetForProfile(profile);
  about_signin_internals->OnAuthenticationResultReceived("Successful");

  SigninClient* signin_client =
      ChromeSigninClientFactory::GetForProfile(profile);
  std::string signin_scoped_device_id =
      signin_client->GetSigninScopedDeviceId();
  base::WeakPtr<InlineSigninHelperDelegate> delegate_weak_ptr;
  if (params.delegate)
    delegate_weak_ptr = params.delegate->GetWeakPtr();

  // InlineSigninHelper will delete itself.
  new InlineSigninHelper(
      delegate_weak_ptr, params.context_getter, profile, status,
      params.signin_access_point, params.signin_reason, params.email,
      params.gaia_id, params.password, params.session_index, params.auth_code,
      signin_scoped_device_id, params.choose_what_to_sync,
      params.confirm_untrusted_signin, params.is_force_sign_in_with_usermanager,
      params.is_auto_closable, params.should_show_account_management);

  // If opened from user manager to unlock a profile, make sure the user manager
  // is closed and that the profile is marked as unlocked.
  if (!params.is_force_sign_in_with_usermanager) {
    UnlockProfileAndHideLoginUI(params.profile_path, params.delegate);
  }
}

InlineSigninHelper::InlineSigninHelper(
    base::WeakPtr<InlineSigninHelperDelegate> delegate,
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
    bool should_show_account_management)
    : gaia_auth_fetcher_(this, GaiaConstants::kChromeSource, context_getter),
      delegate_(delegate),
      profile_(profile),
      create_status_(create_status),
      signin_access_point_(signin_access_point),
      signin_reason_(signin_reason),
      email_(email),
      gaia_id_(gaia_id),
      password_(password),
      session_index_(session_index),
      auth_code_(auth_code),
      choose_what_to_sync_(choose_what_to_sync),
      confirm_untrusted_signin_(confirm_untrusted_signin),
      is_force_sign_in_with_usermanager_(is_force_sign_in_with_usermanager),
      is_auto_closable_(is_auto_closable),
      should_show_account_management_(should_show_account_management) {
  DCHECK(profile_);
  DCHECK(!email_.empty());
  if (!auth_code_.empty()) {
    gaia_auth_fetcher_.StartAuthCodeForOAuth2TokenExchangeWithDeviceId(
        auth_code, signin_scoped_device_id);
  } else {
    DCHECK(!session_index_.empty());
    gaia_auth_fetcher_
        .DeprecatedStartCookieForOAuthLoginTokenExchangeWithDeviceId(
            session_index_, signin_scoped_device_id);
  }
}

InlineSigninHelper::~InlineSigninHelper() {}

void InlineSigninHelper::OnClientOAuthSuccess(const ClientOAuthResult& result) {
  if (is_force_sign_in_with_usermanager_) {
    // If user sign in in UserManager with force sign in enabled, the browser
    // window won't be opened until now.
    profiles::OpenBrowserWindowForProfile(
        base::Bind(&InlineSigninHelper::OnClientOAuthSuccessAndBrowserOpened,
                   base::Unretained(this), result),
        true, false, profile_, create_status_);
  } else {
    OnClientOAuthSuccessAndBrowserOpened(result, profile_, create_status_);
  }
}

void InlineSigninHelper::OnClientOAuthSuccessAndBrowserOpened(
    const ClientOAuthResult& result,
    Profile* profile,
    Profile::CreateStatus status) {
  if (is_force_sign_in_with_usermanager_)
    UnlockProfileAndHideLoginUI(profile_->GetPath(), delegate_.get());
  Browser* browser = chrome::FindLastActiveWithProfile(profile_);
  AboutSigninInternals* about_signin_internals =
      AboutSigninInternalsFactory::GetForProfile(profile_);
  about_signin_internals->OnRefreshTokenReceived("Successful");

  // Prime the account tracker with this combination of gaia id/display email.
  std::string account_id =
      AccountTrackerServiceFactory::GetForProfile(profile_)->SeedAccountInfo(
          gaia_id_, email_);

  SigninManager* signin_manager = SigninManagerFactory::GetForProfile(profile_);
  std::string primary_email =
      signin_manager->GetAuthenticatedAccountInfo().email;
  if (gaia::AreEmailsSame(email_, primary_email) &&
      (signin_reason_ == signin_metrics::Reason::REASON_REAUTHENTICATION ||
       signin_reason_ == signin_metrics::Reason::REASON_UNLOCK) &&
      !password_.empty() && profiles::IsLockAvailable(profile_)) {
    LocalAuth::SetLocalAuthCredentials(profile_, password_);
  }

  if (signin_reason_ == signin_metrics::Reason::REASON_REAUTHENTICATION ||
      signin_reason_ == signin_metrics::Reason::REASON_UNLOCK ||
      signin_reason_ == signin_metrics::Reason::REASON_ADD_SECONDARY_ACCOUNT) {
    ProfileOAuth2TokenServiceFactory::GetForProfile(profile_)
        ->UpdateCredentials(account_id, result.refresh_token);

    if (is_auto_closable_) {
      // Close the gaia sign in tab via a task to make sure we aren't in the
      // middle of any webui handler code.
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE,
          base::BindOnce(&InlineSigninHelperDelegate::CloseTab, delegate_,
                         should_show_account_management_));
    }

    if (signin_reason_ == signin_metrics::Reason::REASON_REAUTHENTICATION ||
        signin_reason_ == signin_metrics::Reason::REASON_UNLOCK) {
      signin_manager->MergeSigninCredentialIntoCookieJar();
    }
    LogSigninReason(signin_reason_);
  } else {
    browser_sync::ProfileSyncService* sync_service =
        ProfileSyncServiceFactory::GetForProfile(profile_);
    SigninErrorController* error_controller =
        SigninErrorControllerFactory::GetForProfile(profile_);

    OneClickSigninSyncStarter::StartSyncMode start_mode =
        OneClickSigninSyncStarter::CONFIRM_SYNC_SETTINGS_FIRST;
    if (signin_access_point_ ==
            signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS ||
        choose_what_to_sync_) {
      bool show_settings_without_configure =
          error_controller->HasError() && sync_service &&
          sync_service->IsFirstSetupComplete();
      if (!show_settings_without_configure)
        start_mode = OneClickSigninSyncStarter::CONFIGURE_SYNC_FIRST;
    }

    OneClickSigninSyncStarter::ConfirmationRequired confirmation_required =
        confirm_untrusted_signin_
            ? OneClickSigninSyncStarter::CONFIRM_UNTRUSTED_SIGNIN
            : OneClickSigninSyncStarter::CONFIRM_AFTER_SIGNIN;

    bool start_signin = !HandleCrossAccountError(
        result.refresh_token, confirmation_required, start_mode);
    if (start_signin) {
      CreateSyncStarter(browser, signin_access_point_, signin_reason_,
                        result.refresh_token,
                        OneClickSigninSyncStarter::CURRENT_PROFILE, start_mode,
                        confirmation_required);
      base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
    }
  }
}

void InlineSigninHelper::CreateSyncStarter(
    Browser* browser,
    signin_metrics::AccessPoint signin_access_point,
    signin_metrics::Reason signin_reason,
    const std::string& refresh_token,
    OneClickSigninSyncStarter::ProfileMode profile_mode,
    OneClickSigninSyncStarter::StartSyncMode start_mode,
    OneClickSigninSyncStarter::ConfirmationRequired confirmation_required) {
  // OneClickSigninSyncStarter will delete itself once the job is done.
  new OneClickSigninSyncStarter(
      profile_, browser, gaia_id_, email_, password_, refresh_token,
      signin_access_point, signin_reason, profile_mode, start_mode,
      confirmation_required,
      base::Bind(&InlineSigninHelperDelegate::SyncStarterCallback, delegate_));
}

bool InlineSigninHelper::HandleCrossAccountError(
    const std::string& refresh_token,
    OneClickSigninSyncStarter::ConfirmationRequired confirmation_required,
    OneClickSigninSyncStarter::StartSyncMode start_mode) {
  // With force sign in enabled, cross account
  // sign in will be rejected in the early stage so there is no need to show the
  // warning page here.
  if (signin_util::IsForceSigninEnabled())
    return false;

  std::string last_email =
      profile_->GetPrefs()->GetString(prefs::kGoogleServicesLastUsername);

  // TODO(skym): Warn for high risk upgrade scenario, crbug.com/572754.
  if (!IsCrossAccountError(profile_, email_, gaia_id_))
    return false;

  Browser* browser = chrome::FindLastActiveWithProfile(profile_);
  content::WebContents* web_contents =
      browser->tab_strip_model()->GetActiveWebContents();

  SigninEmailConfirmationDialog::AskForConfirmation(
      web_contents, profile_, last_email, email_,
      base::Bind(&InlineSigninHelper::ConfirmEmailAction,
                 base::Unretained(this), web_contents, refresh_token,
                 confirmation_required, start_mode));
  return true;
}

void InlineSigninHelper::ConfirmEmailAction(
    content::WebContents* web_contents,
    const std::string& refresh_token,
    OneClickSigninSyncStarter::ConfirmationRequired confirmation_required,
    OneClickSigninSyncStarter::StartSyncMode start_mode,
    SigninEmailConfirmationDialog::Action action) {
  Browser* browser = chrome::FindLastActiveWithProfile(profile_);
  switch (action) {
    case SigninEmailConfirmationDialog::CREATE_NEW_USER:
      base::RecordAction(
          base::UserMetricsAction("Signin_ImportDataPrompt_DontImport"));
      CreateSyncStarter(browser, signin_access_point_, signin_reason_,
                        refresh_token, OneClickSigninSyncStarter::NEW_PROFILE,
                        start_mode, confirmation_required);
      break;
    case SigninEmailConfirmationDialog::START_SYNC:
      base::RecordAction(
          base::UserMetricsAction("Signin_ImportDataPrompt_ImportData"));
      CreateSyncStarter(browser, signin_access_point_, signin_reason_,
                        refresh_token,
                        OneClickSigninSyncStarter::CURRENT_PROFILE, start_mode,
                        confirmation_required);
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

void InlineSigninHelper::OnClientOAuthFailure(
    const GoogleServiceAuthError& error) {
  if (delegate_)
    delegate_->HandleLoginError(error.ToString(), base::string16());

  AboutSigninInternals* about_signin_internals =
      AboutSigninInternalsFactory::GetForProfile(profile_);
  about_signin_internals->OnRefreshTokenReceived("Failure");

  base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
}


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
  account_id_(account_id) {
  DCHECK(profile_);
  DCHECK(browser_);
  DCHECK(!account_id_.empty());
  
  // Should not start synching if the profile is already authenticated
  DCHECK(SigninManagerFactory::GetForProfile(profile_)->IsAuthenticated());

  // Force sign-in uses the modal sign-in flow.
  DCHECK(!signin_util::IsForceSigninEnabled());

  switch (signin_reason_) {
  case signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT:
    break;
  case signin_metrics::Reason::REASON_ADD_SECONDARY_ACCOUNT:
  case signin_metrics::Reason::REASON_REAUTHENTICATION:
  case  signin_metrics::Reason::REASON_UNLOCK:
  case signin_metrics::Reason::REASON_UNKNOWN_REASON:
    NOTREACHED();
    break;
  case  signin_metrics::Reason::REASON_MAX:
    NOTREACHED();

  }

  if (CanTurnSyncOn())
    StartTurnOnSync();
}

DiceTurnSyncOnHelper::~DiceTurnSyncOnHelper() {}

void DiceTurnSyncOnHelper::StartTurnOnSync() {
  Browser* browser = chrome::FindLastActiveWithProfile(profile_);
  SigninManager* signin_manager = SigninManagerFactory::GetForProfile(profile_);
  std::string primary_email =
    signin_manager->GetAuthenticatedAccountInfo().email;

    bool start_signin = !HandleCrossAccountError();
    if (start_signin) {
      CreateSyncStarter(OneClickSigninSyncStarter::CURRENT_PROFILE);
      base::ThreadTaskRunnerHandle::Get()->DeleteSoon(FROM_HERE, this);
    }
}

bool DiceTurnSyncOnHelper::CanTurnSyncOn() {
  std::string error_msg;
  bool can_offer = CanOffer(profile_, CAN_OFFER_FOR_ALL, account_info_.gaia,
    account_info_.email, &error_msg);
  if (!can_offer) {
     delegate_->HandleLoginError(error_msg,base::UTF8ToUTF16(account_info_.email));
    return false;
  }
  return true;
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


