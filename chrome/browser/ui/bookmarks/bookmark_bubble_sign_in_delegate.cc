// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/bookmarks/bookmark_bubble_sign_in_delegate.h"

#include "chrome/browser/signin/signin_promo.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/signin/dice_turn_sync_on_helper.h"

BookmarkBubbleSignInDelegate::BookmarkBubbleSignInDelegate(Browser* browser)
    : browser_(browser),
      profile_(browser->profile()) {
  BrowserList::AddObserver(this);
}

BookmarkBubbleSignInDelegate::~BookmarkBubbleSignInDelegate() {
  BrowserList::RemoveObserver(this);
}

void BookmarkBubbleSignInDelegate::OnSignInLinkClicked() {
  ShowSignin();
}

void BookmarkBubbleSignInDelegate::ShowSignin() {
  EnsureBrowser();
  chrome::ShowBrowserSignin(
      browser_, signin_metrics::AccessPoint::ACCESS_POINT_BOOKMARK_BUBBLE);
}

void BookmarkBubbleSignInDelegate::EnableSync(const AccountInfo& account) {
  // DiceTurnSyncOnHelper is suicidal (it will kill itself once it finishes
  // enabling sync).
  new DiceTurnSyncOnHelper(
      profile_, browser_,
      signin_metrics::AccessPoint::ACCESS_POINT_BOOKMARK_BUBBLE,
      signin_metrics::Reason::REASON_SIGNIN_PRIMARY_ACCOUNT, account.account_id,
      DiceTurnSyncOnHelper::SigninAbortedMode::KEEP_ACCOUNT);
}

void BookmarkBubbleSignInDelegate::OnBrowserRemoved(Browser* browser) {
  if (browser == browser_)
    browser_ = NULL;
}

void BookmarkBubbleSignInDelegate::EnsureBrowser() {
  if (!browser_) {
    Profile* original_profile = profile_->GetOriginalProfile();
    browser_ = chrome::FindLastActiveWithProfile(original_profile);
    if (!browser_) {
      browser_ = new Browser(Browser::CreateParams(original_profile, true));
    }
  }
}
