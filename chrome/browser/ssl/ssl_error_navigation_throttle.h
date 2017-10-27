// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_SSL_ERROR_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_SSL_SSL_ERROR_NAVIGATION_THROTTLE_H_

#include <memory>
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ssl/ssl_cert_reporter.h"
#include "components/security_interstitials/content/security_interstitial_page.h"
#include "content/public/browser/navigation_throttle.h"

namespace content {
class NavigationHandle;
}  // namespace content

// SSLErrorNavigationThrottle watches for failed navigations that should be
// displayed as SSL interstitial pages.
// More specifically, SSLErrorNavigationThrottle::WillFailRequest() will defer
// any navigations that failed due to a certificate error. After calculating
// which interstitial to show, it will cancel the navigation with the
// interstitial's custom error page HTML.
class SSLErrorNavigationThrottle : public content::NavigationThrottle {
 public:
  explicit SSLErrorNavigationThrottle(
      content::NavigationHandle* handle,
      std::unique_ptr<SSLCertReporter> ssl_cert_reporter);
  ~SSLErrorNavigationThrottle() override;

  // content::NavigationThrottle:
  ThrottleCheckResult WillFailRequest() override;
  const char* GetNameForLogging() override;

  void ShowInterstitial(
      std::unique_ptr<security_interstitials::SecurityInterstitialPage>
          blocking_page);

 private:
  void HandleSSLError();

  std::unique_ptr<SSLCertReporter> ssl_cert_reporter_;
  base::WeakPtrFactory<SSLErrorNavigationThrottle> weak_ptr_factory_;
};

#endif  // CHROME_BROWSER_SSL_SSL_ERROR_NAVIGATION_THROTTLE_H_
