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
namespace {

bool IsValidWebPaymentsGURL(const GURL& url,
                            bool allow_localhost_http,
                            std::string* error_message_suffix) {
  if (!url.is_valid()) {
    *error_message_suffix = "is not a valid URL.";
    return false;
  }

  if (!url.SchemeIs(url::kHttpsScheme)) {
    if (url.SchemeIs(url::kHttpScheme)) {
      if (!allow_localhost_http || !net::IsLocalhost(url)) {
        *error_message_suffix =
            std::string(
                "is not an absolute URL with HTTPS scheme (or HTTP "
                "scheme for localhost with --") +
            switches::kAllowWebPaymentsLocalhostUrls + " command line flag).";
        return false;
      }
    } else {
      *error_message_suffix =
          std::string(
              "is not an absoluate URL with HTTPS scheme (or HTTP "
              "scheme for localhost with --") +
          switches::kAllowWebPaymentsLocalhostUrls + " command line flag).";
      return false;
    }
  }

  if (url.has_password() || url.has_username()) {
    *error_message_suffix = "contains username or password.";
    return false;
  }

  return true;
}

}  // namespace

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
  return url.is_valid() && (net::IsLocalhost(url) || url.SchemeIsFile());
}

// static
bool OriginSecurityChecker::IsValidWebPaymentsUrl(
    const std::string& url_string,
    const GURL& base_url,
    GURL* url,
    std::string* error_message_suffix) {
  bool allow_localhost_http = base::CommandLine::ForCurrentProcess()->HasSwitch(
      switches::kAllowWebPaymentsLocalhostUrls);

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

  static const char kHttpPrefix[] = "http://";
  static const char kHttpsPrefix[] = "https://";

  if (!base::StartsWith(url_string, kHttpsPrefix,
                        base::CompareCase::INSENSITIVE_ASCII) &&
      (!allow_localhost_http ||
       !base::StartsWith(url_string, kHttpPrefix,
                         base::CompareCase::INSENSITIVE_ASCII))) {
    std::string ignored;
    if (!IsValidWebPaymentsGURL(base_url, allow_localhost_http, &ignored)) {
      *error_message_suffix =
          std::string(
              "is not an absolute URL with HTTPS scheme (or HTTP "
              "scheme for localhost with --") +
          switches::kAllowWebPaymentsLocalhostUrls + " command line flag).";
      return false;
    }

    *url = base_url.Resolve(url_string);
    if (!url->is_valid()) {
      *error_message_suffix = std::string(
                                  "is not a relative URL and not an absolute "
                                  "URL with HTTPS scheme (or HTTP "
                                  "scheme for localhost with --") +
                              switches::kAllowWebPaymentsLocalhostUrls +
                              " command line flag).";
      return false;
    }
    return true;
  }

  *url = GURL(url_string);
  return IsValidWebPaymentsGURL(*url, allow_localhost_http,
                                error_message_suffix);
}

}  // namespace payments
