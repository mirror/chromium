// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth_handler.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "net/base/net_errors.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/log/net_log_event_type.h"

namespace net {

HttpAuthHandler::HttpAuthHandler() : target_(HttpAuth::AUTH_NONE) {}

HttpAuthHandler::~HttpAuthHandler() {
}

bool HttpAuthHandler::InitFromChallenge(HttpAuthChallengeTokenizer* challenge,
                                        HttpAuth::Target target,
                                        const SSLInfo& ssl_info,
                                        const GURL& origin,
                                        const NetLogWithSource& net_log) {
  origin_ = origin;
  target_ = target;
  net_log_ = net_log;

  auth_challenge_ = challenge->challenge_text();
  bool ok = Init(challenge, ssl_info);

  // Init() is expected to set the scheme, realm, and properties. The realm may
  // be empty.
  DCHECK_IMPLIES(ok,
                 HttpUtil::IsToken(auth_scheme_.begin(), auth_scheme_.end()) &&
                     base::ToLowerASCII(auth_scheme_) == auth_scheme_);
  return ok;
}

bool HttpAuthHandler::NeedsIdentity() {
  return true;
}

bool HttpAuthHandler::AllowsDefaultCredentials() {
  return false;
}

bool HttpAuthHandler::AllowsExplicitCredentials() {
  return true;
}

}  // namespace net
