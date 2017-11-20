// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/process_dice_header_observer_impl.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/dice_tab_helper.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/sync/one_click_signin_sync_starter.h"
#include "components/signin/core/browser/profile_management_switches.h"
#include "components/signin/core/browser/signin_manager.h"

#include "base/metrics/user_metrics.h"
#include "base/metrics/user_metrics_action.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/signin/about_signin_internals_factory.h"
#include "chrome/browser/signin/account_tracker_service_factory.h"
#include "chrome/browser/signin/investigator_dependency_provider.h"
#include "chrome/browser/signin/local_auth.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_error_controller_factory.h"
#include "chrome/browser/signin/signin_util.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/signin/finish_login.h"
#include "chrome/browser/ui/webui/signin/signin_email_confirmation_dialog.h"
#include "components/browser_sync/profile_sync_service.h"
#include "components/signin/core/browser/about_signin_internals.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "components/signin/core/browser/signin_pref_names.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/webui/signin/login_ui_service.h"
#include "chrome/browser/ui/webui/signin/login_ui_service_factory.h"
#include "chrome/common/webui_url_constants.h"

namespace {
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

}  // namespace

ProcessDiceHeaderObserverImpl::ProcessDiceHeaderObserverImpl(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), should_start_sync_(false) {}

bool ProcessDiceHeaderObserverImpl::WillStartRefreshTokenFetch(
    const std::string& gaia_id,
    const std::string& email,
    const std::string& auth_code) {
  if (!web_contents())
    return true;

  if (!signin::IsDicePrepareMigrationEnabled())
    return true;

  DiceTabHelper* tab_helper = DiceTabHelper::FromWebContents(web_contents());
  should_start_sync_ =
      tab_helper && tab_helper->should_start_sync_after_web_signin();

  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  DCHECK(browser);
  Profile* profile = browser->profile();
  DCHECK(profile);

  if (should_start_sync_) {
    FinishLogin(
        FinishCompleteLoginParams(
            this, profile->GetRequestContext(),
            tab_helper->signin_access_point(), tab_helper->signin_reason(),
            base::FilePath(), false, email, gaia_id, "" /* password */,
            "" /* session_index */, auth_code, true, false, false, false),
        profile, Profile::CREATE_STATUS_CREATED);
  }
  return !should_start_sync_;
}

void ProcessDiceHeaderObserverImpl::DidFinishRefreshTokenFetch(
    const std::string& gaia_id,
    const std::string& email) {
  //  content::WebContents* web_contents = this->web_contents();
  //  if (!web_contents || !should_start_sync_) {
  //    VLOG(1) << "Do not start sync after web sign-in.";
  //    return;
  //  }
  //  if (!signin::IsDicePrepareMigrationEnabled())
  //    return;
  //
  //  Browser* browser = chrome::FindBrowserWithWebContents(web_contents);
  //  DCHECK(browser);
  //  Profile* profile = browser->profile();
  //  DCHECK(profile);
  //  SigninManager* signin_manager =
  //  SigninManagerFactory::GetForProfile(profile); DCHECK(signin_manager); if
  //  (signin_manager->IsAuthenticated()) {
  //    VLOG(1) << "Do not start sync after web sign-in [already
  //    authenticated]."; return;
  //  }
  //
  //  DiceTabHelper* tab_helper = DiceTabHelper::FromWebContents(web_contents);
  //  DCHECK(tab_helper);
  //
  //  // OneClickSigninSyncStarter is suicidal (it will kill itself once it
  //  finishes
  //  // enabling sync).
  //  VLOG(1) << "Start sync after web sign-in.";
  //  new InlineSigninHelper(
  //    profile, browser, web_contents, tab_helper->signin_access_point(),
  //    tab_helper->signin_reason(), Profile::CREATE_STATUS_CREATED, email,
  //    gaia_id, "", "", "", true, true, false);
  //
  //  //new OneClickSigninSyncStarter(
  //  //    profile, browser, gaia_id, email, tab_helper->signin_access_point(),
  //  //    tab_helper->signin_reason(), OneClickSigninSyncStarter::Callback());

  LogHistogramValue(signin_metrics::HISTOGRAM_ACCEPTED);
  LogHistogramValue(signin_metrics::HISTOGRAM_WITH_DEFAULTS);

}

void ProcessDiceHeaderObserverImpl::CloseLoginDialog() {
  // No need to close any dialog.
}

void ProcessDiceHeaderObserverImpl::HandleLoginError(
    const std::string& error_msg,
    const base::string16& email) {
  Browser* browser = chrome::FindBrowserWithWebContents(web_contents());
  DCHECK(browser);
  Profile* profile = browser->profile();
  DCHECK(profile);

  SyncStarterCallback(OneClickSigninSyncStarter::SYNC_SETUP_FAILURE);

  if (!error_msg.empty()) {
    LoginUIServiceFactory::GetForProfile(profile)->DisplayLoginResult(
        browser, base::UTF8ToUTF16(error_msg), email);
  }
}

void ProcessDiceHeaderObserverImpl::CloseTab(bool show_account_management) {
  NOTREACHED();
}

void ProcessDiceHeaderObserverImpl::SyncStarterCallback(
    OneClickSigninSyncStarter::SyncSetupResult result) {
  content::WebContents* web_contents = this->web_contents();
  if (!web_contents || !should_start_sync_) {
    VLOG(1) << "Do not start sync after web sign-in.";
    return;
  }
  if (!signin::IsDicePrepareMigrationEnabled())
    return;

  DiceTabHelper* tab_helper = DiceTabHelper::FromWebContents(web_contents);
  DCHECK(tab_helper);
  RedirectToNtpOrAppsPage(web_contents, tab_helper->signin_access_point());
}
