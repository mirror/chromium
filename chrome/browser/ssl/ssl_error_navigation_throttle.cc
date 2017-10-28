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
#include "net/cert/cert_status_flags.h"

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
  DCHECK(base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kCommittedInterstitials));

  content::NavigationHandle* handle = navigation_handle();
  int net_error =
      net::MapCertStatusToNetError(handle->GetSSLInfo().value().cert_status);

  if (!net::IsCertificateError(net_error)) {
    return content::NavigationThrottle::PROCEED;
  }

  // SSLErrorHandler may take several seconds to calculate what interstitial to
  // show, so we post a task to start these calculations after deferring.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &SSLErrorHandler::HandleSSLError, handle->GetWebContents(), net_error,
          handle->GetSSLInfo().value(), handle->GetURL(),
          handle->ShouldSSLErrorsBeFatal(), false,
          std::move(ssl_cert_reporter_),
          base::Callback<void(content::CertificateRequestResultType)>(),
          base::BindOnce(&SSLErrorNavigationThrottle::ShowInterstitial,
                         weak_ptr_factory_.GetWeakPtr())));

  return content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::DEFER);
}

void SSLErrorNavigationThrottle::ShowInterstitial(
    std::unique_ptr<security_interstitials::SecurityInterstitialPage>
        blocking_page) {
  std::string error_page_html = blocking_page->GetHTMLContents();

  SSLErrorTabHelper::AssociateBlockingPage(
      navigation_handle()->GetWebContents(),
      navigation_handle()->GetNavigationId(), std::move(blocking_page));

  CancelDeferredNavigation(content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::CANCEL,
      navigation_handle()->GetNetErrorCode(), error_page_html));
}
