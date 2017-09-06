// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_error_navigation_throttle.h"

#include "base/bind.h"
#include "base/optional.h"
#include "chrome/browser/safe_browsing/certificate_reporting_service.h"
#include "chrome/browser/safe_browsing/certificate_reporting_service_factory.h"
#include "chrome/browser/ssl/ssl_error_handler.h"
#include "components/security_interstitials/core/ssl_error_ui.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "net/base/net_errors.h"
#include "net/ssl/ssl_info.h"

namespace {

// An implementation of the SSLCertReporter interface used by
// SSLErrorHandler. Uses CertificateReportingService to send reports. The
// service handles queueing and re-sending of failed reports. Each certificate
// error creates a new instance of this class.
//
// TODO: Remove the copy of this in chrome_content_browser_client.cc
class CertificateReportingServiceCertReporter : public SSLCertReporter {
 public:
  explicit CertificateReportingServiceCertReporter(
      CertificateReportingService* service)
      : service_(service) {}
  ~CertificateReportingServiceCertReporter() override {}

  // SSLCertReporter implementation
  void ReportInvalidCertificateChain(
      const std::string& serialized_report) override {
    service_->Send(serialized_report);
  }

 private:
  CertificateReportingService* service_;

  DISALLOW_COPY_AND_ASSIGN(CertificateReportingServiceCertReporter);
};

// TODO
void EmptyCallback(content::CertificateRequestResultType type) {}

}  // namespace

SSLErrorNavigationThrottle::SSLErrorNavigationThrottle(
    content::NavigationHandle* navigation_handle)
    : content::NavigationThrottle(navigation_handle) {}

SSLErrorNavigationThrottle::~SSLErrorNavigationThrottle() {}

const char* SSLErrorNavigationThrottle::GetNameForLogging() {
  return "SSLErrorNavigationThrottle";
}

content::NavigationThrottle::ThrottleCheckResult
SSLErrorNavigationThrottle::WillFailRequest() {
  if (!SSLErrorHandler::AreCommittedInterstitialsEnabled()) {
    return content::NavigationThrottle::ThrottleCheckResult(
        content::NavigationThrottle::PROCEED);
  }

  if (!net::IsCertificateError(navigation_handle()->GetNetErrorCode())) {
    return content::NavigationThrottle::ThrottleCheckResult(
        content::NavigationThrottle::PROCEED);
  }

  HandleSSLError();

  return content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::DEFER);
}

void SSLErrorNavigationThrottle::ShowInterstitial(std::string error_page_html) {
  LOG(ERROR) << error_page_html;
  // content::NavigationThrottle::ThrottleCheckResult(
  //     content::NavigationThrottle::CANCEL,
  //     static_cast<int>(handle->GetNetErrorCode()),
  //     error_page_html);
  // navigation_handle()->CancelDeferredNavigation(this, result);
}

void SSLErrorNavigationThrottle::HandleSSLError() {
  DCHECK(net::IsCertificateError(navigation_handle()->GetNetErrorCode()));
  DCHECK(navigation_handle()->GetSSLInfo().has_value());

  content::NavigationHandle* handle = navigation_handle();

  // TODO: Remove the copy of this in chrome_content_browser_client.cc
  int options_mask = 0;
  // if (overridable) // TODO
  //   options_mask |=
  //   security_interstitials::SSLErrorUI::SOFT_OVERRIDE_ENABLED;
  if (handle->ShouldSSLErrorsBeFatal())
    options_mask |= security_interstitials::SSLErrorUI::STRICT_ENFORCEMENT;
  // if (expired_previous_decision) // TODO
  //   options_mask |=
  //   security_interstitials::SSLErrorUI::EXPIRED_BUT_PREVIOUSLY_ALLOWED;

  content::WebContents* web_contents = handle->GetWebContents();
  CertificateReportingService* cert_reporting_service =
      CertificateReportingServiceFactory::GetForBrowserContext(
          web_contents->GetBrowserContext());
  std::unique_ptr<CertificateReportingServiceCertReporter> cert_reporter(
      new CertificateReportingServiceCertReporter(cert_reporting_service));

  SSLErrorHandler::HandleSSLError(
      web_contents, static_cast<int>(handle->GetNetErrorCode()),
      handle->GetSSLInfo().value(), handle->GetURL(), options_mask,
      std::move(cert_reporter), base::Bind(&EmptyCallback), this);
}
