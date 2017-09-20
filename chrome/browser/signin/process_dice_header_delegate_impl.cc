// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/process_dice_header_delegate_impl.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/dice_tab_helper.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/sync/one_click_signin_sync_starter.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/common/profile_management_switches.h"

ProcessDiceHeaderDelegateImpl::ProcessDiceHeaderDelegateImpl(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents), should_start_sync_(false) {
  DCHECK(signin::IsAccountConsistencyDiceEnabled());
}

void ProcessDiceHeaderDelegateImpl::WillStartRefreshTokenFetch(
    const std::string& gaia_id,
    const std::string& email) {
  if (!web_contents())
    return;

  DiceTabHelper* tab_helper = DiceTabHelper::FromWebContents(web_contents());
  should_start_sync_ =
      tab_helper && tab_helper->should_start_sync_after_web_signin();
}

void ProcessDiceHeaderDelegateImpl::DidFinishRefreshTokenFetch(
    const std::string& gaia_id,
    const std::string& email) {
  if (!web_contents() || !should_start_sync_) {
    VLOG(1) << "Do not start sync after web sign-in.";
    return;
  }

  Browser* browser = Browser::FromWebContents(web_contents());
  Profile* profile = browser->profile();
  SigninManager* signin_manager = SigninManagerFactory::GetForProfile(profile);
  if (signin_manager->IsAuthenticated()) {
    VLOG(1) << "Do not start sync after web sign-in [already authenticated].";
    return;
  }

  // OneClickSigninSyncStarter is suicidal (it will kill itself once it finishes
  // enabling sync).
  VLOG(1) << "Start sync after web sign-in.";
  new OneClickSigninSyncStarter(profile, browser, gaia_id, email,
                                web_contents(),
                                OneClickSigninSyncStarter::Callback());
}
