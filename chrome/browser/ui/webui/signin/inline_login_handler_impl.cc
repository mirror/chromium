// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/signin/inline_login_handler_impl.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/location.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_window.h"
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
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog.h"
#include "chrome/browser/ui/tab_modal_confirm_dialog_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/user_manager.h"
#include "chrome/browser/ui/webui/signin/finish_login.h"
#include "chrome/browser/ui/webui/signin/login_ui_service.h"
#include "chrome/browser/ui/webui/signin/login_ui_service_factory.h"
#include "chrome/browser/ui/webui/signin/signin_utils.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/prefs/pref_service.h"
#include "components/signin/core/browser/about_signin_internals.h"
#include "components/signin/core/browser/account_tracker_service.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_error_controller.h"
#include "components/signin/core/browser/signin_header_helper.h"
#include "components/signin/core/browser/signin_investigator.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_ui.h"
#include "google_apis/gaia/gaia_auth_fetcher.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "google_apis/gaia/gaia_constants.h"
#include "google_apis/gaia/gaia_urls.h"
#include "net/base/url_util.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

void LogHistogramValue(signin_metrics::AccessPointAction action) {
  UMA_HISTOGRAM_ENUMERATION("Signin.AllAccessPointActions", action,
                            signin_metrics::HISTOGRAM_MAX);
}

// Returns true if |profile| is a system profile or created from one.
bool IsSystemProfile(Profile* profile) {
  return profile->GetOriginalProfile()->IsSystemProfile();
}

void RedirectToNtpOrAppsPage(content::WebContents* contents,
                             signin_metrics::AccessPoint access_point) {
  // Do nothing if a navigation is pending, since this call can be triggered
  // from DidStartLoading. This avoids deleting the pending entry while we are
  // still navigating to it. See crbug/346632.
  if (contents->GetController().GetPendingEntry())
    return;

  VLOG(1) << "RedirectToNtpOrAppsPage";
  // Redirect to NTP/Apps page and display a confirmation bubble
  GURL url(access_point ==
                   signin_metrics::AccessPoint::ACCESS_POINT_APPS_PAGE_LINK
               ? chrome::kChromeUIAppsURL
               : chrome::kChromeUINewTabURL);
  content::OpenURLParams params(url, content::Referrer(),
                                WindowOpenDisposition::CURRENT_TAB,
                                ui::PAGE_TRANSITION_AUTO_TOPLEVEL, false);
  contents->OpenURL(params);
}

void RedirectToNtpOrAppsPageIfNecessary(
    content::WebContents* contents,
    signin_metrics::AccessPoint access_point) {
  if (access_point != signin_metrics::AccessPoint::ACCESS_POINT_SETTINGS)
    RedirectToNtpOrAppsPage(contents, access_point);
}

void CloseModalSigninIfNeeded(InlineLoginHandlerImpl* handler) {
  if (handler) {
    Browser* browser = handler->GetDesktopBrowser();
    if (browser)
      browser->signin_view_controller()->CloseModalSignin();
  }
}

}  // namespace

InlineLoginHandlerImpl::InlineLoginHandlerImpl()
      : confirm_untrusted_signin_(false),
        weak_factory_(this) {
}

InlineLoginHandlerImpl::~InlineLoginHandlerImpl() {}

// This method is not called with webview sign in enabled.
void InlineLoginHandlerImpl::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!web_contents() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsErrorPage()) {
    return;
  }

  // Returns early if this is not a gaia webview navigation.
  content::RenderFrameHost* gaia_frame =
      signin::GetAuthFrame(web_contents(), "signin-frame");
  if (navigation_handle->GetRenderFrameHost() != gaia_frame)
    return;

  // Loading any untrusted (e.g., HTTP) URLs in the privileged sign-in process
  // will require confirmation before the sign in takes effect.
  const GURL kGaiaExtOrigin(
      GaiaUrls::GetInstance()->signin_completed_continue_url().GetOrigin());
  if (!navigation_handle->GetURL().is_empty()) {
    GURL origin(navigation_handle->GetURL().GetOrigin());
    if (navigation_handle->GetURL().spec() != url::kAboutBlankURL &&
        origin != kGaiaExtOrigin &&
        !gaia::IsGaiaSignonRealm(origin)) {
      confirm_untrusted_signin_ = true;
    }
  }
}

void InlineLoginHandlerImpl::SetExtraInitParams(base::DictionaryValue& params) {
  params.SetString("service", "chromiumsync");

  // If this was called from the user manager to reauthenticate the profile,
  // make sure the webui is aware.
  Profile* profile = Profile::FromWebUI(web_ui());
  if (IsSystemProfile(profile))
    params.SetBoolean("dontResizeNonEmbeddedPages", true);

  content::WebContents* contents = web_ui()->GetWebContents();
  const GURL& current_url = contents->GetURL();
  signin_metrics::Reason reason =
      signin::GetSigninReasonForPromoURL(current_url);

  std::string is_constrained;
  net::GetValueForKeyInQuery(current_url, "constrained", &is_constrained);

  // Use new embedded flow if in constrained window.
  if (is_constrained == "1") {
    const GURL& url = GaiaUrls::GetInstance()->embedded_signin_url();
    params.SetBoolean("isNewGaiaFlow", true);
    params.SetString("clientId",
                     GaiaUrls::GetInstance()->oauth2_chrome_client_id());
    params.SetString("gaiaPath", url.path().substr(1));

    std::string flow;
    switch (reason) {
      case signin_metrics::Reason::REASON_ADD_SECONDARY_ACCOUNT:
        flow = "addaccount";
        break;
      case signin_metrics::Reason::REASON_REAUTHENTICATION:
      case signin_metrics::Reason::REASON_UNLOCK:
        flow = "reauth";
        break;
      default:
        flow = "signin";
        break;
    }
    params.SetString("flow", flow);
  }

  content::WebContentsObserver::Observe(contents);
  LogHistogramValue(signin_metrics::HISTOGRAM_SHOWN);
  UMA_HISTOGRAM_BOOLEAN("Signin.UseDeprecatedGaiaSigninEndpoint",
                        is_constrained == "1");
}

void InlineLoginHandlerImpl::CompleteLogin(const base::ListValue* args) {
  content::WebContents* contents = web_ui()->GetWebContents();
  const GURL& current_url = contents->GetURL();

  const base::DictionaryValue* dict = NULL;
  args->GetDictionary(0, &dict);

  bool skip_for_now = false;
  dict->GetBoolean("skipForNow", &skip_for_now);
  if (skip_for_now) {
    signin::SetUserSkippedPromo(Profile::FromWebUI(web_ui()));
    SyncStarterCallback(OneClickSigninSyncStarter::SYNC_SETUP_FAILURE);
    return;
  }

  // This value exists only for webview sign in.
  bool trusted = false;
  if (dict->GetBoolean("trusted", &trusted))
    confirm_untrusted_signin_ = !trusted;

  base::string16 email_string16;
  dict->GetString("email", &email_string16);
  DCHECK(!email_string16.empty());
  std::string email(base::UTF16ToASCII(email_string16));

  base::string16 password_string16;
  dict->GetString("password", &password_string16);
  std::string password(base::UTF16ToASCII(password_string16));

  base::string16 gaia_id_string16;
  dict->GetString("gaiaId", &gaia_id_string16);
  DCHECK(!gaia_id_string16.empty());
  std::string gaia_id = base::UTF16ToASCII(gaia_id_string16);

  std::string is_constrained;
  net::GetValueForKeyInQuery(current_url, "constrained", &is_constrained);
  const bool is_password_separated_signin_flow = is_constrained == "1";

  base::string16 session_index_string16;
  dict->GetString("sessionIndex", &session_index_string16);
  std::string session_index = base::UTF16ToASCII(session_index_string16);
  DCHECK(is_password_separated_signin_flow || !session_index.empty());

  base::string16 auth_code_string16;
  dict->GetString("authCode", &auth_code_string16);
  std::string auth_code = base::UTF16ToASCII(auth_code_string16);
  DCHECK(!is_password_separated_signin_flow || !auth_code.empty());

  bool choose_what_to_sync = false;
  dict->GetBoolean("chooseWhatToSync", &choose_what_to_sync);

  content::StoragePartition* partition =
      content::BrowserContext::GetStoragePartitionForSite(
          contents->GetBrowserContext(), signin::GetSigninPartitionURL());

  // Extract params from |current_url|.
  signin_metrics::AccessPoint access_point =
      signin::GetAccessPointForPromoURL(current_url);
  signin_metrics::Reason reason =
      signin::GetSigninReasonForPromoURL(current_url);
  bool is_auto_closable = signin::IsAutoCloseEnabledInURL(current_url);
  bool should_show_account_management =
      signin::ShouldShowAccountManagement(current_url);

  // If this was called from the user manager to reauthenticate the profile,
  // the current profile is the system profile.  In this case, use the email to
  // find the right profile to reauthenticate.  Otherwise the profile can be
  // taken from web_ui().
  Profile* profile = Profile::FromWebUI(web_ui());
  if (IsSystemProfile(profile)) {
    ProfileManager* manager = g_browser_process->profile_manager();
    base::FilePath path = profiles::GetPathOfProfileWithEmail(manager, email);
    if (path.empty()) {
      path = UserManager::GetSigninProfilePath();
    }
    if (!path.empty()) {
      signin_metrics::Reason reason =
          signin::GetSigninReasonForPromoURL(current_url);
      // If we are only reauthenticating a profile in the user manager (and not
      // unlocking it), load the profile and finish the login.
      if (reason == signin_metrics::Reason::REASON_REAUTHENTICATION) {
        FinishCompleteLoginParams params(
            this, partition->GetURLRequestContext(), access_point, reason,
            base::FilePath(), confirm_untrusted_signin_, email, gaia_id,
            password, session_index, auth_code, choose_what_to_sync, false,
            is_auto_closable, should_show_account_management);
        ProfileManager::CreateCallback callback = base::Bind(
            &InlineLoginHandlerImpl::FinishCompleteLogin, params, current_url);
        profiles::LoadProfileAsync(path, callback);
      } else {
        // Otherwise, switch to the profile and finish the login. Pass the
        // profile path so it can be marked as unlocked. Don't pass a handler
        // pointer since it will be destroyed before the callback runs.
        bool is_force_signin_enabled = signin_util::IsForceSigninEnabled();
        InlineLoginHandlerImpl* handler = nullptr;
        if (is_force_signin_enabled)
          handler = this;
        FinishCompleteLoginParams params(
            handler, partition->GetURLRequestContext(), access_point, reason,
            path, confirm_untrusted_signin_, email, gaia_id, password,
            session_index, auth_code, choose_what_to_sync,
            is_force_signin_enabled, is_auto_closable,
            should_show_account_management);
        ProfileManager::CreateCallback callback = base::Bind(
            &InlineLoginHandlerImpl::FinishCompleteLogin, params, current_url);
        if (is_force_signin_enabled) {
          // Browser window will be opened after ClientOAuthSuccess.
          profiles::LoadProfileAsync(path, callback);
        } else {
          profiles::SwitchToProfile(path, true, callback,
                                    ProfileMetrics::SWITCH_PROFILE_UNLOCK);
        }
      }
    }
  } else {
    FinishCompleteLogin(
        FinishCompleteLoginParams(
            this, partition->GetURLRequestContext(), access_point, reason,
            base::FilePath(), confirm_untrusted_signin_, email, gaia_id,
            password, session_index, auth_code, choose_what_to_sync, false,
            is_auto_closable, should_show_account_management),
        current_url, profile, Profile::CREATE_STATUS_CREATED);
  }
}

// static
void InlineLoginHandlerImpl::FinishCompleteLogin(
    const FinishCompleteLoginParams& params,
    const GURL& current_url,
    Profile* profile,
    Profile::CreateStatus status) {
  // When doing a SAML sign in, this email check may result in a false
  // positive.  This happens when the user types one email address in the
  // gaia sign in page, but signs in to a different account in the SAML sign in
  // page.
  std::string default_email;
  std::string validate_email;
  if (net::GetValueForKeyInQuery(current_url, "email", &default_email) &&
      net::GetValueForKeyInQuery(current_url, "validateEmail",
                                 &validate_email) &&
      validate_email == "1" && !default_email.empty()) {
    if (!gaia::AreEmailsSame(params.email, default_email)) {
      if (params.delegate) {
        params.delegate->HandleLoginError(
            l10n_util::GetStringFUTF8(IDS_SYNC_WRONG_EMAIL,
                                      base::UTF8ToUTF16(default_email)),
            base::UTF8ToUTF16(params.email));
      }
      return;
    }
  }

  FinishLogin(params, profile, status);
}

void InlineLoginHandlerImpl::CloseLoginDialog() {
  CloseDialogFromJavascript();
}

void InlineLoginHandlerImpl::HandleLoginError(const std::string& error_msg,
                                              const base::string16& email) {
  SyncStarterCallback(OneClickSigninSyncStarter::SYNC_SETUP_FAILURE);
  Browser* browser = GetDesktopBrowser();
  Profile* profile = Profile::FromWebUI(web_ui());

  if (IsSystemProfile(profile))
    profile = g_browser_process->profile_manager()->GetProfileByPath(
        UserManager::GetSigninProfilePath());
  CloseModalSigninIfNeeded(this);
  if (!error_msg.empty()) {
    LoginUIServiceFactory::GetForProfile(profile)->DisplayLoginResult(
        browser, base::UTF8ToUTF16(error_msg), email);
  }
}

Browser* InlineLoginHandlerImpl::GetDesktopBrowser() {
  Browser* browser = chrome::FindBrowserWithWebContents(
      web_ui()->GetWebContents());
  if (!browser)
    browser = chrome::FindLastActiveWithProfile(Profile::FromWebUI(web_ui()));
  return browser;
}

void InlineLoginHandlerImpl::SyncStarterCallback(
    OneClickSigninSyncStarter::SyncSetupResult result) {
  content::WebContents* contents = web_ui()->GetWebContents();

  if (contents->GetController().GetPendingEntry()) {
    // Do nothing if a navigation is pending, since this call can be triggered
    // from DidStartLoading. This avoids deleting the pending entry while we are
    // still navigating to it. See crbug/346632.
    return;
  }

  const GURL& current_url = contents->GetLastCommittedURL();
  signin_metrics::AccessPoint access_point =
      signin::GetAccessPointForPromoURL(current_url);
  bool auto_close = signin::IsAutoCloseEnabledInURL(current_url);

  if (result == OneClickSigninSyncStarter::SYNC_SETUP_FAILURE) {
    RedirectToNtpOrAppsPage(contents, access_point);
  } else if (auto_close) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&InlineLoginHandlerImpl::CloseTab,
                       weak_factory_.GetWeakPtr(),
                       signin::ShouldShowAccountManagement(current_url)));
  } else {
    RedirectToNtpOrAppsPageIfNecessary(contents, access_point);
  }
}

void InlineLoginHandlerImpl::CloseTab(bool show_account_management) {
  content::WebContents* tab = web_ui()->GetWebContents();
  Browser* browser = chrome::FindBrowserWithWebContents(tab);
  if (browser) {
    TabStripModel* tab_strip_model = browser->tab_strip_model();
    if (tab_strip_model) {
      int index = tab_strip_model->GetIndexOfWebContents(tab);
      if (index != TabStripModel::kNoTab) {
        tab_strip_model->ExecuteContextMenuCommand(
            index, TabStripModel::CommandCloseTab);
      }
    }

    if (show_account_management) {
      browser->window()->ShowAvatarBubbleFromAvatarButton(
          BrowserWindow::AVATAR_BUBBLE_MODE_ACCOUNT_MANAGEMENT,
          signin::ManageAccountsParams(),
          signin_metrics::AccessPoint::ACCESS_POINT_AVATAR_BUBBLE_SIGN_IN,
          false);
    }
  }
}
