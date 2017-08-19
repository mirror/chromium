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
  base::Optional<bool> fatal_cert_error =
      navigation_handle()->GetFatalCertError();

  if (net::IsCertificateError(net)) {
    DCHECK(ssl_info.has_value());
    DCHECK(fatal_cert_error.has_value());
  }

  return content::NavigationThrottle::PROCEED;
}