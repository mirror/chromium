// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_error_tab_helper.h"
#include "base/strings/string_number_conversions.h"
#include "components/security_interstitials/content/security_interstitial_page.h"
#include "content/public/browser/navigation_handle.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(SSLErrorTabHelper);

SSLErrorTabHelper::~SSLErrorTabHelper() {}

void SSLErrorTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->IsSameDocument()) {
    return;
  }

  auto it = blocking_pages_for_navigations_.find(
      navigation_handle->GetNavigationId());

  if (navigation_handle->HasCommitted()) {
    if (blocking_page_for_currently_committed_navigation_) {
      blocking_page_for_currently_committed_navigation_
          ->OnInterstitialClosing();
    }

    if (it == blocking_pages_for_navigations_.end()) {
      blocking_page_for_currently_committed_navigation_.reset();
    } else {
      blocking_page_for_currently_committed_navigation_ = std::move(it->second);
    }
  }

  if (it != blocking_pages_for_navigations_.end()) {
    blocking_pages_for_navigations_.erase(it);
  }
}

void SSLErrorTabHelper::WebContentsDestroyed() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->OnInterstitialClosing();
  }
}

// static
void SSLErrorTabHelper::AssociateBlockingPage(
    content::WebContents* web_contents,
    int64_t navigation_id,
    std::unique_ptr<security_interstitials::SecurityInterstitialPage>
        blocking_page) {
  // CreateForWebContents() creates a tab helper if it doesn't exist for
  // |web_contents| yet.
  SSLErrorTabHelper::CreateForWebContents(web_contents);

  SSLErrorTabHelper* helper = SSLErrorTabHelper::FromWebContents(web_contents);
  helper->SetBlockingPage(navigation_id, std::move(blocking_page));
}

security_interstitials::SecurityInterstitialPage*
SSLErrorTabHelper::GetBlockingPageForCurrentlyCommittedNavigationForTesting() {
  return blocking_page_for_currently_committed_navigation_.get();
}

SSLErrorTabHelper::SSLErrorTabHelper(content::WebContents* web_contents)
    : WebContentsObserver(web_contents), binding_(web_contents, this) {}

void SSLErrorTabHelper::SetBlockingPage(
    int64_t navigation_id,
    std::unique_ptr<security_interstitials::SecurityInterstitialPage>
        blocking_page) {
  blocking_pages_for_navigations_[navigation_id] = std::move(blocking_page);
}

void SSLErrorTabHelper::ReceiveMessage(
    interstitial_commands::mojom::InterstitialCommands::InterstitialCommandId
        cmd) {
  if (blocking_page_for_currently_committed_navigation_) {
    // The value will be converted to a string because CommandReceived expects
    // a string that it then turns back into an integer, this redundant
    // conversion will be removed  once commited interstitials are the only
    // supported codepath.
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString((int)cmd));
  }
}
