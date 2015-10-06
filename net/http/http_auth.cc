// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth.h"

#include <algorithm>

#include "base/basictypes.h"
#include "base/bind.h"
#include "base/strings/string_tokenizer.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "net/base/net_errors.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/http_auth_handler.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_auth_scheme.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"

namespace net {

namespace {

// Hardcoded map of HTTP authentication scheme priorities. Higher priorities are
// preferred over lower.
struct SchemePriority {
  const char* scheme;
  int priority;
} kSchemeScores[] = {
    {"basic", 1},
    {"digest", 2},
    {"ntlm", 3},
    {"negotiate", 4},
};

// Priority assigned to unknown authentication schemes. They are currently
// ranked lower than Basic, which might be a bit too conservative.
const int kSchemePriorityDefault = 0;

// Not a valid priority.
const int kSchemePriorityInvalid = -1;

// Higher priority schemes are preferred over lower priority schemes.
int GetSchemePriority(const std::string& scheme) {
  DCHECK(HttpAuth::IsValidNormalizedScheme(scheme));
  for (const auto& iter : kSchemeScores) {
    if (scheme == iter.scheme)
      return iter.priority;
  }
  return kSchemePriorityDefault;
}

std::unique_ptr<base::Value> AuthHandlerCreationFailureParams(
    const std::string* challenge,
    int error,
    NetLogCaptureMode capture_mode) {
  std::unique_ptr<base::DictionaryValue> dict(new base::DictionaryValue());
  dict->SetString("challenge", *challenge);
  dict->SetInteger("net_error", error);
  return dict.Pass();
}

}  // namespace

HttpAuth::Identity::Identity() : source(IDENT_SRC_NONE), invalid(true) {}

HttpAuth::Identity::~Identity() {}

// static
void HttpAuth::ChooseBestChallenge(
    HttpAuthHandlerFactory* http_auth_handler_factory,
    const HttpResponseHeaders& response_headers,
    const SSLInfo& ssl_info,
    Target target,
    const GURL& origin,
    const HttpAuthSchemeSet& disabled_schemes,
    const BoundNetLog& net_log,
    std::unique_ptr<HttpAuthHandler>* handler) {
  DCHECK(http_auth_handler_factory);
  DCHECK(!handler->get());

  int best_priority = kSchemePriorityInvalid;
  std::unique_ptr<HttpAuthHandler> best;
  const std::string header_name = GetChallengeHeaderName(target);
  std::string cur_challenge;
  size_t iter = 0;
  while (headers->EnumerateHeader(&iter, header_name, &cur_challenge)) {
    HttpAuthChallengeTokenizer challenge_tokenizer(cur_challenge.begin(),
                                                   cur_challenge.end());
    std::string cur_auth_scheme = challenge_tokenizer.NormalizedScheme();
    if (cur_auth_scheme.empty())
      continue;

    if (disabled_schemes.Contains(cur_auth_scheme))
      continue;

    // Always prefer the highest priority scheme available. If two handlers are
    // of equal priority, then the handler based on the earlier challenge wins.
    int cur_priority = GetSchemePriority(cur_auth_scheme);
    if (best.get() && best_priority >= cur_priority)
      continue;

    // Immediately trying to create a handler may be wasteful because we may see
    // a higher ranking challenge later on in the headers. However, this is
    // rare. Servers typically specify higher priority schemes before lower
    // priority schemes.
    std::unique_ptr<HttpAuthHandler> current_handler;
    int rv = http_auth_handler_factory->CreateAuthHandler(
        &challenge_tokenizer, target, ssl_info, origin,
        HttpAuthHandlerFactory::CREATE_CHALLENGE, 1, net_log, &current_handler);
    if (rv != OK) {
      net_log.AddEvent(
          NetLog::TYPE_AUTH_HANDLER_CREATION_FAILURE,
          base::Bind(&AuthHandlerCreationFailureParams, &cur_challenge, rv));
      continue;
    }
    DCHECK(current_handler.get());
    DCHECK_EQ(cur_auth_scheme, current_handler->auth_scheme());
    best.swap(current_handler);
    best_priority = cur_priority;
  }
  handler->swap(best);
}

// static
HttpAuth::AuthorizationResult HttpAuth::HandleChallengeResponse(
    HttpAuthHandler* handler,
    const HttpResponseHeaders& response_headers,
    Target target,
    const HttpAuthSchemeSet& disabled_schemes,
    std::string* challenge_used) {
  DCHECK(handler);
  DCHECK(challenge_used);
  challenge_used->clear();
  const std::string& current_scheme = handler->auth_scheme();
  if (disabled_schemes.Contains(current_scheme))
    return HttpAuth::AUTHORIZATION_RESULT_REJECT;
  const std::string header_name = GetChallengeHeaderName(target);
  size_t iter = 0;
  std::string challenge;
  HttpAuth::AuthorizationResult authorization_result =
      HttpAuth::AUTHORIZATION_RESULT_INVALID;
  while (response_headers.EnumerateHeader(&iter, header_name, &challenge)) {
    HttpAuthChallengeTokenizer props(challenge.begin(), challenge.end());
    if (!props.SchemeIs(current_scheme))
      continue;
    authorization_result = handler->HandleAnotherChallenge(props);
    if (authorization_result != HttpAuth::AUTHORIZATION_RESULT_INVALID) {
      *challenge_used = challenge;
      return authorization_result;
    }
  }
  // Finding no matches is equivalent to rejection.
  return HttpAuth::AUTHORIZATION_RESULT_REJECT;
}

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
