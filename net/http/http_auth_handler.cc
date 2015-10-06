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
                                        const BoundNetLog& net_log) {
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

namespace {

NetLogEventType EventTypeFromAuthTarget(HttpAuth::Target target) {
  switch (target) {
    case HttpAuth::AUTH_PROXY:
      return NetLogEventType::AUTH_PROXY;
    case HttpAuth::AUTH_SERVER:
      return NetLogEventType::AUTH_SERVER;
    default:
      NOTREACHED();
      return NetLogEventType::CANCELLED;
  }
}

}  // namespace

int HttpAuthHandler::GenerateAuthToken(
    const AuthCredentials* credentials, const HttpRequestInfo* request,
    const CompletionCallback& callback, std::string* auth_token) {
  DCHECK(!callback.is_null());
  DCHECK(request);
  DCHECK(credentials != NULL || AllowsDefaultCredentials());
  DCHECK(auth_token != NULL);
  DCHECK(callback_.is_null());
  callback_ = callback;
  net_log_.BeginEvent(EventTypeFromAuthTarget(target_));
  int rv = GenerateAuthTokenImpl(
      credentials, request,
      base::Bind(&HttpAuthHandler::OnGenerateAuthTokenComplete,
                 base::Unretained(this)),
      auth_token);
  if (rv != ERR_IO_PENDING)
    FinishGenerateAuthToken();
  return rv;
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

void HttpAuthHandler::OnGenerateAuthTokenComplete(int rv) {
  CompletionCallback callback = callback_;
  FinishGenerateAuthToken();
  DCHECK(!callback.is_null());
  callback.Run(rv);
}

void HttpAuthHandler::FinishGenerateAuthToken() {
  // TOOD(cbentzel): Should this be done in OK case only?
  net_log_.EndEvent(EventTypeFromAuthTarget(target_));
  callback_.Reset();
}

}  // namespace net
