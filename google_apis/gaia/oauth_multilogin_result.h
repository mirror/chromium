// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GOOGLE_APIS_GAIA_OAUTH_MULTILOGIN_RESULT_H_
#define GOOGLE_APIS_GAIA_OAUTH_MULTILOGIN_RESULT_H_

#include <string>

#include "base/time/time.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "net/cookies/cookie_constants.h"
#include "net/url_request/url_request_status.h"
#include "url/gurl.h"

struct OAuthMultiloginResult {
  enum class MultiloginStatus {
    kOk,

    // Network errors.
    kCanceled,
    kFailed,

    // Errors returned from server.
    kRetry,
    kError,
    kInvalidInput,
    kInvalidTokens
  };

  enum class AccountStatus { kOk, kRecoverable, kNotRecoverable };

  struct FailedAccount {
    std::string gaia_id;
    AccountStatus status;
    GURL url;
  };

  struct Cookie {
    Cookie();
    ~Cookie();

    std::string name;
    std::string value;
    std::string host;
    bool host_only;
    std::string path;
    bool secure;
    bool http_only;
    net::CookieSameSite same_site;
    bool session;
    base::Time expiration_date;
  };

  OAuthMultiloginResult(const std::string& data,
                        const net::URLRequestStatus& status,
                        int response_code);
  ~OAuthMultiloginResult();

  MultiloginStatus status;
  std::vector<Cookie> cookies;
  std::vector<gaia::ListedAccount> accounts;
};

#endif  // GOOGLE_APIS_GAIA_OAUTH_MULTILOGIN_RESULT_H_
