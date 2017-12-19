// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_error_tab_helper.h"
#include "base/strings/string_number_conversions.h"
#include "components/security_interstitials/content/security_interstitial_page.h"
#include "components/security_interstitials/core/controller_client.h"
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

void SSLErrorTabHelper::DontProceed() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString(
            security_interstitials::SecurityInterstitialCommand::
                CMD_DONT_PROCEED));
  }
}

void SSLErrorTabHelper::Proceed() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString(
            security_interstitials::SecurityInterstitialCommand::CMD_PROCEED));
  }
}

void SSLErrorTabHelper::ShowMoreSection() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString(
            security_interstitials::SecurityInterstitialCommand::
                CMD_SHOW_MORE_SECTION));
  }
}

void SSLErrorTabHelper::OpenHelpCenter() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString(
            security_interstitials::SecurityInterstitialCommand::
                CMD_OPEN_HELP_CENTER));
  }
}

void SSLErrorTabHelper::OpenDiagnostic() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString(
            security_interstitials::SecurityInterstitialCommand::
                CMD_OPEN_DIAGNOSTIC));
  }
}

void SSLErrorTabHelper::Reload() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString(
            security_interstitials::SecurityInterstitialCommand::CMD_RELOAD));
  }
}

void SSLErrorTabHelper::OpenDateSettings() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString(
            security_interstitials::SecurityInterstitialCommand::
                CMD_OPEN_DATE_SETTINGS));
  }
}

void SSLErrorTabHelper::OpenLogin() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString(security_interstitials::
                                 SecurityInterstitialCommand::CMD_OPEN_LOGIN));
  }
}

void SSLErrorTabHelper::DoReport() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString(security_interstitials::
                                 SecurityInterstitialCommand::CMD_DO_REPORT));
  }
}

void SSLErrorTabHelper::DontReport() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString(security_interstitials::
                                 SecurityInterstitialCommand::CMD_DONT_REPORT));
  }
}

void SSLErrorTabHelper::OpenReportingPrivacy() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString(
            security_interstitials::SecurityInterstitialCommand::
                CMD_OPEN_REPORTING_PRIVACY));
  }
}

void SSLErrorTabHelper::OpenWhitepaper() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString(
            security_interstitials::SecurityInterstitialCommand::
                CMD_OPEN_WHITEPAPER));
  }
}

void SSLErrorTabHelper::ReportPhishingError() {
  if (blocking_page_for_currently_committed_navigation_) {
    blocking_page_for_currently_committed_navigation_->CommandReceived(
        base::NumberToString(
            security_interstitials::SecurityInterstitialCommand::
                CMD_REPORT_PHISHING_ERROR));
  }
}
