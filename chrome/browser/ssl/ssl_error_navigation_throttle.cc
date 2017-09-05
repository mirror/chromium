// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_error_navigation_throttle.h"

#include "base/bind.h"
#include "base/optional.h"
#include "chrome/browser/safe_browsing/certificate_reporting_service.h"
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
  // TODO: Put logic here, behind a flag.
  net::Error net_error = navigation_handle()->GetNetErrorCode();

  if (!net::IsCertificateError(net_error)) {
    return content::NavigationThrottle::ThrottleCheckResult(
        content::NavigationThrottle::PROCEED);
  }

  if (!SSLErrorHandler::AreCommittedInterstitialsEnabled()) {
    return content::NavigationThrottle::ThrottleCheckResult(
        content::NavigationThrottle::PROCEED);
  }

  DCHECK(navigation_handle()->GetSSLInfo().has_value());
  DCHECK(navigation_handle()->ShouldSSLErrorsBeFatal().has_value());

  // Since this is a net error, GetSSLInfo() and ShouldSSLErrorsBeFatal() must
  // have values.
  // base::Optional<net::SSLInfo> ssl_info =
  // navigation_handle()->GetSSLInfo().value(); base::Optional<net::SSLInfo>
  // should_ssl_errors_be_fatal =
  //     navigation_handle()->ShouldSSLErrorsBeFatal().value();

  // content::NavigationThrottle::ThrottleCheckResult result(
  //     content::NavigationThrottle::CANCEL);
  // result.net_error = net_error;
  // result.error_page_url = GURL(
  //     "data:text/html,<!doctype html>\n"
  //     "<html><head><style>\n"
  //     "  body,html{height:100%;margin:0;background:#f2f2f2;}\n"
  //     "  body{display:flex;justify-content:center;align-items:center;\n"
  //     "    font-family:sans-serif;font-size:12vw;font-variant:small-caps;\n"
  //     "    color:#C63626;}\n"
  //     "</style></head>\n"
  //     "<body>&#x1F614; Error Page</body></html>\n");

  // Otherwise, display an SSL blocking page. The interstitial page takes
  // ownership of ssl_blocking_page.
  // TODO: Remove the copy of this in chrome_content_browser_client.cc
  int options_mask = 0;
  // if (overridable) // TODO
  //   options_mask |= SSLErrorUI::SOFT_OVERRIDE_ENABLED;
  if (fatal)
    options_mask |= SSLErrorUI::STRICT_ENFORCEMENT;
  // if (expired_previous_decision) // TODO
  //   options_mask |= SSLErrorUI::EXPIRED_BUT_PREVIOUSLY_ALLOWED;

  content::NavigationHandle* handle = navigation_handle();
  WebContents* web_contents = handle->GetWebContents();

  CertificateReportingService* cert_reporting_service =
      CertificateReportingServiceFactory::GetForBrowserContext(
          web_contents->GetBrowserContext());
  std::unique_ptr<CertificateReportingServiceCertReporter> cert_reporter(
      new CertificateReportingServiceCertReporter(cert_reporting_service));

  SSLErrorHandler::HandleSSLError(
      web_contents, static_cast<int>(handle->GetNetErrorCode()),
      handler->GetSSLInfo().value(), handle->GetURL(), options_mask,
      std::move(cert_reporter), base::Bind(&EmptyCallback));

  return content::NavigationThrottle::ThrottleCheckResult(
      content::NavigationThrottle::DEFER);
}
