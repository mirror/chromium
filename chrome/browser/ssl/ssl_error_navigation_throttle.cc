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

  if (!net::IsCertificateError(net_error)) {
    return content::NavigationThrottle::ThrottleCheckResult(
        content::NavigationThrottle::PROCEED);
  }

  // Since this is a net error, GetSSLInfo() and ShouldSSLErrorsBeFatal() must
  // have values.
  // base::Optional<net::SSLInfo> ssl_info =
  // navigation_handle()->GetSSLInfo().value(); base::Optional<net::SSLInfo>
  // should_ssl_errors_be_fatal =
  //     navigation_handle()->ShouldSSLErrorsBeFatal().value();

  content::NavigationThrottle::ThrottleCheckResult result(
      content::NavigationThrottle::CANCEL);
  result.net_error = net_error;
  result.error_page_html = std::string(
      "<!doctype html>\n"
      "<html><head><style>\n"
      "  body,html{height:100%;margin:0;background:#f2f2f2;}\n"
      "  body{display:flex;justify-content:center;align-items:center;\n"
      "    font-family:sans-serif;font-size:12vw;font-variant:small-caps;\n"
      "    color:#C63626;}\n"
      "</style></head>\n"
      "<body>&#x1F614; Error Page</body></html>\n");

  return result;
}
