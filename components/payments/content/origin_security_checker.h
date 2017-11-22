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

  // Returns true for a valid |url_string| for web payments, such as URL-based
  // payment method identifier, supported origin, or web app manifest location.
  // For web payments, these all must be valid URLs with HTTPS scheme or HTTP
  // scheme for localhost when --allow-web-payments-localhost-urls command line
  // flag is present.
  //
  // When the function returns true, |url| is set to the parsed URL. When the
  // function returns false, the |error_message_suffix| is set to the string
  // that can be used in a developer facing error message. Sample usage:
  //
  //   std::string method = "https://example.com/webpay";
  //   GURL url;
  //   std::string error_message_suffix;
  //   if (!IsValidUrlBasedPaymentMethod(method, &url, &error_message_suffix))
  //     LOG(ERROR) << "The payment method " << error_message_suffix;
  //
  // This could print, for example:
  //
  //  "The payment method is not a valid URL with HTTPS scheme or HTTP scheme
  //  (for localhost with --allow-web-payments-localhost-urls command line
  //  flag)."
  static bool IsValidWebPaymentsUrl(const std::string& url_string,
                                    GURL* url,
                                    std::string* error_message_suffix);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(OriginSecurityChecker);
};

}  // namespace payments

#endif  // COMPONENTS_PAYMENTS_CONTENT_ORIGIN_SECURITY_CHECKER_H_
