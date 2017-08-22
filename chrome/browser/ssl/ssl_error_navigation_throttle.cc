// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ssl/ssl_error_navigation_throttle.h"

#include "base/optional.h"
#include "content/public/browser/navigation_handle.h"
#include "net/base/net_errors.h"
#include "net/ssl/ssl_info.h"

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
  base::Optional<net::SSLInfo> ssl_info = navigation_handle()->GetSSLInfo();
  bool should_ssl_errors_be_fatal =
      navigation_handle()->ShouldSSLErrorsBeFatal();

  if (net::IsCertificateError(net_error)) {
    DCHECK(ssl_info.has_value());
    LOG(ERROR) << "net_error: " << net_error;
    LOG(ERROR) << "ssl_info->cert_status: " << ssl_info->cert_status;
    LOG(ERROR) << "should_ssl_errors_be_fatal: " << should_ssl_errors_be_fatal;
  } else {
    LOG(ERROR) << "Not a cert error: " << net_error;
  }

  // TODO: Call CancelDeferredNavigationWithErrorURL().

  return content::NavigationThrottle::PROCEED;
}
