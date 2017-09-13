// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/dice_tab_helper.h"

#include "base/logging.h"
#include "chrome/browser/signin/dice_tab_helper.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/sync/one_click_signin_sync_starter.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "google_apis/gaia/gaia_urls.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(DiceTabHelper);

DiceTabHelper::DiceTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      should_start_sync_after_web_signin_(true) {
  DCHECK_EQ(GaiaUrls::GetInstance()->add_account_url(),
            content::WebContentsObserver::web_contents()->GetVisibleURL());
}

DiceTabHelper::~DiceTabHelper() {}

void DiceTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  LOG(ERROR) << "Start Navigation to " << navigation_handle->GetURL();
  if (!navigation_handle->IsInMainFrame()) {
    // Subframe navigations do not affect |should_start_sync_after_web_signin_|.
    return;
  }
  if (navigation_handle->GetURL().GetOrigin() !=
      GaiaUrls::GetInstance()->gaia_url()) {
    // Avoid starting sync if the user navigates outsite of the GAIA domain.
    should_start_sync_after_web_signin_ = false;
    return;
  }

  if (!navigation_handle->IsRendererInitiated()) {
    // Avoid starting sync if the navigations comes from the browser.
    should_start_sync_after_web_signin_ = false;
  }
}

void DiceTabHelper::StartSyncIfNeeded(const std::string& gaia_id,
                                      const std::string& email) {
  DCHECK(web_contents());
  if (!should_start_sync_after_web_signin_)
    return;

  Browser* browser = Browser::FromWebContents(web_contents());
  Profile* profile = browser->profile();
  // OneClickSigninSyncStarter is suicidal (it will kill itself once it finishes
  // enabling sync).
  new OneClickSigninSyncStarter(profile, browser, gaia_id, email,
                                web_contents(),
                                OneClickSigninSyncStarter::Callback());
}
