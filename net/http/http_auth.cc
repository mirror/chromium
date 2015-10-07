// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth.h"

#include "base/basictypes.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "net/base/net_errors.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/http_auth_handler.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_scheme.h"
#include "net/http/http_request_headers.h"

namespace net {

HttpAuth::Identity::Identity() : source(IDENT_SRC_NONE), invalid(true) {}

HttpAuth::Identity::~Identity() {}

// static
std::string HttpAuth::GetChallengeHeaderName(Target target) {
  switch (target) {
    case AUTH_PROXY:
      return "Proxy-Authenticate";
    case AUTH_SERVER:
      return "WWW-Authenticate";
    default:
      NOTREACHED();
      return std::string();
  }
}

// static
std::string HttpAuth::GetAuthorizationHeaderName(Target target) {
  switch (target) {
    case AUTH_PROXY:
      return HttpRequestHeaders::kProxyAuthorization;
    case AUTH_SERVER:
      return HttpRequestHeaders::kAuthorization;
    default:
      NOTREACHED();
      return std::string();
  }
}

// static
std::string HttpAuth::GetAuthTargetString(Target target) {
  switch (target) {
    case AUTH_PROXY:
      return "proxy";
    case AUTH_SERVER:
      return "server";
    default:
      NOTREACHED();
      return std::string();
  }
}

// static
bool HttpAuth::IsValidNormalizedScheme(const std::string& scheme) {
  return HttpUtil::IsToken(scheme.begin(), scheme.end()) &&
         base::ToLowerASCII(scheme) == scheme;
}

}  // namespace net
