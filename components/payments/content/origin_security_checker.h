// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PAYMENTS_CONTENT_ORIGIN_SECURITY_CHECKER_H_
#define COMPONENTS_PAYMENTS_CONTENT_ORIGIN_SECURITY_CHECKER_H_

#include <string>

#include "base/macros.h"

class GURL;

namespace payments {

class OriginSecurityChecker {
 public:
  // Returns true for a valid |url| from a secure origin.
  static bool IsOriginSecure(const GURL& url);

  // Returns true for a valid |url| with a cryptographic scheme, e.g., HTTPS,
  // HTTPS-SO, WSS.
  static bool IsSchemeCryptographic(const GURL& url);

  // Returns true for a valid |url| with localhost or file:// scheme origin.
  static bool IsOriginLocalhostOrFile(const GURL& url);

  // Returns true for a valid |url_string| for web payments. For example:
  //  - URL-based payment method identifier
  //  - Supported origin
  //  - Web app manifest location
  //
  // For web payments, these all must be valid URLs with HTTPS scheme. If
  // --allow-web-payments-localhost-urls command line flag is present, then
  // http://localhost is also allowed.
  //
  // When the function returns true, |url| is set to the parsed URL. When the
  // function returns false, the |error_message_suffix| is set to the string
  // that can be used in a developer facing error message. Sample usage:
  //
  //   std::string method = "https://example.com/webpay";
  //   GURL url;
  //   std::string error_message_suffix;
  //   if (!IsValidUrlBasedPaymentMethod(method, GURL(), &url,
  //           &error_message_suffix)) {
  //     LOG(ERROR) << "The payment method " << error_message_suffix;
  //   }
  //
  // This could set |error_message_suffix| to the following, for example:
  //
  //   "The payment method is not a valid URL with HTTPS scheme or HTTP scheme
  //   (for localhost with --allow-web-payments-localhost-urls command line
  //   flag)."
  //
  // If |base_url| is a valid web payments URL, then |url_string| is parsed as
  // relative to |base_url|. For example, if |base_url| is "https://example.com"
  // and |url_string| is "/file.json", then the output |url| will be
  // "https://example.com/file.json".
  static bool IsValidWebPaymentsUrl(const std::string& url_string,
                                    const GURL& base_url,
                                    GURL* url,
                                    std::string* error_message_suffix);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(OriginSecurityChecker);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_ORIGIN_SECURITY_CHECKER_H_
