// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_error_navigation_throttle.h"

#include "base/bind.h"
#include "base/optional.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/safe_browsing/certificate_reporting_service.h"
#include "chrome/browser/safe_browsing/certificate_reporting_service_factory.h"
#include "chrome/browser/ssl/ssl_error_handler.h"
#include "chrome/browser/ssl/ssl_error_tab_helper.h"
#include "chrome/common/chrome_switches.h"
#include "components/security_interstitials/core/ssl_error_ui.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/base/net_errors.h"

namespace {

// TODO: convert users of this callback to accept nullptr.
void EmptyCallback(content::CertificateRequestResultType type) {}

bool AreCommittedInterstitialsEnabled() {
  return base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kCommittedInterstitials);
}

}  // namespace

SSLErrorNavigationThrottle::SSLErrorNavigationThrottle(
    content::NavigationHandle* navigation_handle,
    std::unique_ptr<SSLCertReporter> ssl_cert_reporter)
    : content::NavigationThrottle(navigation_handle),
      ssl_cert_reporter_(std::move(ssl_cert_reporter)),
      weak_ptr_factory_(this) {}

SSLErrorNavigationThrottle::~SSLErrorNavigationThrottle() {}

const char* SSLErrorNavigationThrottle::GetNameForLogging() {
  return "SSLErrorNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
SSLErrorNavigationThrottle::WillFailRequest() {
  if (!AreCommittedInterstitialsEnabled() ||
      !net::IsCertificateError(navigation_handle()->GetNetErrorCode())) {
    return content::NavigationThrottle::ThrottleCheckResult(
        content::NavigationThrottle::PROCEED);
  }

  // SSLErrorHandler may take several seconds to calculate what interstitial to
  // show, so we post a task to start these calculations after deferring.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&SSLErrorNavigationThrottle::HandleSSLError,
                            weak_ptr_factory_.GetWeakPtr()));

  return content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::DEFER);
}

void SSLErrorNavigationThrottle::ShowInterstitial(
    std::unique_ptr<security_interstitials::SecurityInterstitialPage>
        blocking_page) {
  std::string error_page_html = blocking_page->GetHTMLContents();

  content::WebContents* web_contents = navigation_handle()->GetWebContents();
  // CreateForWebContents() creates a tab helper if it doesn't exist for the
  // web_contents yet. FromWebContents() retrieves it.
  SSLErrorTabHelper::CreateForWebContents(web_contents);
  SSLErrorTabHelper::FromWebContents(web_contents)
      ->SetBlockingPageForNavigation(navigation_handle()->GetNavigationId(),
                                     std::move(blocking_page));

  CancelDeferredNavigation(content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::CANCEL,
      navigation_handle()->GetNetErrorCode(), error_page_html));
}

void SSLErrorNavigationThrottle::HandleSSLError() {
  content::NavigationHandle* handle = navigation_handle();

  DCHECK(net::IsCertificateError(handle->GetNetErrorCode()));
  DCHECK(handle->GetSSLInfo().has_value());

  SSLErrorHandler::HandleSSLError(
      handle->GetWebContents(), handle->GetNetErrorCode(),
      handle->GetSSLInfo().value(), handle->GetURL(),
      handle->ShouldSSLErrorsBeFatal(), false, std::move(ssl_cert_reporter_),
      base::Bind(&EmptyCallback),
      base::BindOnce(&SSLErrorNavigationThrottle::ShowInterstitial,
                     weak_ptr_factory_.GetWeakPtr()));
}
