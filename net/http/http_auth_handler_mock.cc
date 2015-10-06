// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth_handler_mock.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/net_errors.h"
#include "net/http/http_auth_challenge_tokenizer.h"
#include "net/http/http_request_info.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

HttpAuthHandlerMock::HttpAuthHandlerMock()
    : expected_auth_scheme_("mock"), weak_factory_(this) {}

HttpAuthHandlerMock::~HttpAuthHandlerMock() {
}

void HttpAuthHandlerMock::SetGenerateExpectation(bool async, int rv) {
  generate_async_ = async;
  generate_rv_ = rv;
}

HttpAuth::AuthorizationResult HttpAuthHandlerMock::HandleAnotherChallenge(
    HttpAuthChallengeTokenizer* challenge) {
  // If we receive a second challenge for a regular scheme, assume it's a
  // rejection. Receiving an empty second challenge when expecting multiple
  // rounds is also considered a rejection.
  if (!expect_multiple_challenges_ || challenge->base64_param().empty())
    return HttpAuth::AUTHORIZATION_RESULT_REJECT;
  if (!challenge->SchemeIs(auth_scheme_))
    return HttpAuth::AUTHORIZATION_RESULT_INVALID;
  auth_token_ = auth_scheme_;
  auth_token_.append(" continuation,");
  auth_token_.append(challenge->base64_param());
  return HttpAuth::AUTHORIZATION_RESULT_ACCEPT;
}

bool HttpAuthHandlerMock::NeedsIdentity() {
  return first_round_;
}

bool HttpAuthHandlerMock::AllowsDefaultCredentials() {
  return allows_default_credentials_;
}

bool HttpAuthHandlerMock::AllowsExplicitCredentials() {
  return allows_explicit_credentials_;
}

bool HttpAuthHandlerMock::Init(HttpAuthChallengeTokenizer* challenge) {
  EXPECT_TRUE(challenge->SchemeIs(expected_auth_scheme_))
      << "Mismatched scheme for challenge: " << challenge->challenge_text();
  EXPECT_TRUE(auth_scheme_.empty()) << "Init was already called.";
  EXPECT_TRUE(HttpAuth::IsValidNormalizedScheme(expected_auth_scheme_))
      << "Invalid expected auth scheme.";
  auth_scheme_ = expected_auth_scheme_;
  auth_token_ = expected_auth_scheme_ + " auth_token";
  if (challenge->params_end() != challenge->params_begin()) {
    auth_token_ += ",";
    auth_token_.append(challenge->params_begin(), challenge->params_end());
  }
  return true;
}

int HttpAuthHandlerMock::GenerateAuthTokenImpl(
    const AuthCredentials* credentials,
    const HttpRequestInfo* request,
    const CompletionCallback& callback,
    std::string* auth_token) {
  first_round_ = false;
  request_url_ = request->url;

  if (!credentials || credentials->Empty()) {
    EXPECT_TRUE(AllowsDefaultCredentials()) << "Credentials must be specified "
                                               "if the handler doesn't support "
                                               "default credentials.";
  } else {
    EXPECT_TRUE(AllowsExplicitCredentials()) << "Explicit credentials can only "
                                                "be specified if the handler "
                                                "supports it.";
  }

  if (generate_async_) {
    EXPECT_TRUE(callback_.is_null()) << "GenerateAuthTokenImpl() called while "
                                        "a pending request is in progress";
    EXPECT_EQ(nullptr, generate_auth_token_buffer_)
        << "GenerateAuthTokenImpl() called before previous token was retrieved";
    callback_ = callback;
    generate_auth_token_buffer_ = auth_token;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&HttpAuthHandlerMock::OnGenerateAuthToken,
                              weak_factory_.GetWeakPtr()));
    return ERR_IO_PENDING;
  } else {
    if (generate_rv_ == OK)
      *auth_token = auth_token_;
    return generate_rv_;
  }
}

void HttpAuthHandlerMock::OnGenerateAuthToken() {
  EXPECT_TRUE(generate_async_);
  EXPECT_TRUE(!callback_.is_null());
  if (generate_rv_ == OK)
    *generate_auth_token_buffer_ = auth_token_;
  generate_auth_token_buffer_ = nullptr;
  CompletionCallback callback = callback_;
  callback_.Reset();
  callback.Run(generate_rv_);
}

HttpAuthHandlerMock::Factory::Factory() {}

HttpAuthHandlerMock::Factory::~Factory() {
}

void HttpAuthHandlerMock::Factory::AddMockHandler(
    std::unique_ptr<HttpAuthHandler> handler,
    CreateReason reason,
    HttpAuth::Target target) {
  if (reason == CREATE_PREEMPTIVE)
    preemptive_handlers_[target].push_back(handler.Pass());
  else
    challenge_handlers_[target].push_back(handler.Pass());
}

bool HttpAuthHandlerMock::Factory::HaveAuthHandlers(
    HttpAuth::Target target) const {
  return !challenge_handlers_[target].empty() ||
         !preemptive_handlers_[target].empty();
}

int HttpAuthHandlerMock::Factory::CreateAuthHandler(
    HttpAuthChallengeTokenizer* challenge,
    HttpAuth::Target target,
    const SSLInfo& ssl_info,
    const GURL& origin,
    CreateReason reason,
    int nonce_count,
    const NetLogWithSource& net_log,
    std::unique_ptr<HttpAuthHandler>* handler) {
  std::vector<std::unique_ptr<HttpAuthHandler>>& handler_list =
      reason == CREATE_PREEMPTIVE ? preemptive_handlers_[target]
                                  : challenge_handlers_[target];
  if (handler_list.empty())
    return ERR_UNEXPECTED;
  std::unique_ptr<HttpAuthHandler> tmp_handler = std::move(handler_list.front());
  handler_list.erase(handler_list.begin());
  if (!tmp_handler->InitFromChallenge(challenge, target, origin, net_log))
    return ERR_INVALID_RESPONSE;
  handler->swap(tmp_handler);
  return OK;
}

}  // namespace net
