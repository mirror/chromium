// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/content/origin_security_checker.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/origin_util.h"
#include "net/base/url_util.h"
#include "url/gurl.h"

namespace payments {

// static
bool OriginSecurityChecker::IsOriginSecure(const GURL& url) {
  return url.is_valid() && content::IsOriginSecure(url);
}

// static
bool OriginSecurityChecker::IsSchemeCryptographic(const GURL& url) {
  return url.is_valid() && url.SchemeIsCryptographic();
}

// static
bool OriginSecurityChecker::IsOriginLocalhostOrFile(const GURL& url) {
  return url.is_valid() &&
         (net::IsLocalhost(url.HostNoBracketsPiece()) || url.SchemeIsFile());
}

// static
bool OriginSecurityChecker::IsValidWebPaymentsUrl(
    const std::string& url_string,
    GURL* url,
    std::string* error_message_suffix) {
  DCHECK(url);
  DCHECK(error_message_suffix);

  if (url_string.empty()) {
    *error_message_suffix = "is an empty string.";
    return false;
  }

  // GURL constructor may crash with some invalid unicode strings.
  if (!base::IsStringUTF8(url_string)) {
    *error_message_suffix = "is not valid unicode.";
    return false;
  }

  bool allow_localhost_http = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kAllowWebPaymentsLocalhostUrls);

  static const char kHttpPrefix[] = "http://";
  static const char kHttpsPrefix[] = "https://";

  if (!base::StartsWith(url_string, kHttpsPrefix,
                        base::CompareCase::INSENSITIVE_ASCII) &&
      (!allow_localhost_http ||
       !base::StartsWith(url_string, kHttpPrefix,
                         base::CompareCase::INSENSITIVE_ASCII))) {
    *error_message_suffix = std::string(
                                "is not a URL with HTTPS scheme (or HTTP "
                                "scheme for localhost with --") +
                            switches::kAllowWebPaymentsLocalhostUrls +
                            " command line flag).";
    return false;
  }

  *url = GURL(url_string);
  if (!url->is_valid()) {
    *error_message_suffix = "is not a valid URL.";
    return false;
  }

  DCHECK(url->SchemeIs(url::kHttpsScheme) ||
         (url->SchemeIs(url::kHttpScheme) && allow_localhost_http));

  if (url->SchemeIs(url::kHttpScheme)) {
    DCHECK(allow_localhost_http);
    if (!net::IsLocalhost(url->HostNoBracketsPiece())) {
      *error_message_suffix = std::string(
                                  "is not a URL with HTTPS scheme (or HTTP "
                                  "scheme for localhost with --") +
                              switches::kAllowWebPaymentsLocalhostUrls +
                              " command line flag).";
      return false;
    }
  }

  if (url->has_password() || url->has_username()) {
    *error_message_suffix = "contains username or password.";
    return false;
  }

  return true;
}

}  // namespace payments
