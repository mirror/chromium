// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_auth_handler_mock.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
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
    : HttpAuthHandler("mock"), weak_factory_(this) {}

HttpAuthHandlerMock::~HttpAuthHandlerMock() {}

void HttpAuthHandlerMock::SetInitExpectation(bool async, int rv) {
  init_async_ = async;
  init_rv_ = rv;
}

void HttpAuthHandlerMock::SetGenerateExpectation(bool async, int rv) {
  generate_async_ = async;
  generate_rv_ = rv;
}

HttpAuth::AuthorizationResult HttpAuthHandlerMock::HandleAnotherChallenge(
    const HttpAuthChallengeTokenizer& challenge) {
  EXPECT_TRUE(challenge.SchemeIs(auth_scheme_))
      << "Auth scheme of challenge doesn't match scheme of AuthHandler.";
  EXPECT_TRUE(initialized_) << __FUNCTION__ << " called without calling Init.";
  EXPECT_TRUE(initialization_succeeded) << __FUNCTION__
                                        << " called after a failed Init.";
  // If we receive a second challenge for a regular scheme, assume it's a
  // rejection. Receiving an empty second challenge when expecting multiple
  // rounds is also considered a rejection.
  base::StringPiece challenge_params(challenge.params_begin(),
                                     challenge.params_end());
  if (!expect_multiple_challenges_ || challenge_params.empty())
    return HttpAuth::AUTHORIZATION_RESULT_REJECT;
  auth_token_ = auth_scheme_;
  auth_token_.append(" continuation,");
  auth_token_.append(challenge.params_begin(), challenge.params_end());
  return HttpAuth::AUTHORIZATION_RESULT_ACCEPT;
}

bool HttpAuthHandlerMock::NeedsIdentity() {
  EXPECT_TRUE(initialized_) << __FUNCTION__ << " called without calling Init.";
  EXPECT_TRUE(initialization_succeeded) << __FUNCTION__
                                        << " called after a failed Init.";
  return first_round_;
}

bool HttpAuthHandlerMock::AllowsDefaultCredentials() {
  EXPECT_TRUE(initialized_) << __FUNCTION__ << " called without calling Init.";
  EXPECT_TRUE(initialization_succeeded) << __FUNCTION__
                                        << " called after a failed Init.";
  return allows_default_credentials_;
}

bool HttpAuthHandlerMock::AllowsExplicitCredentials() {
  EXPECT_TRUE(initialized_) << __FUNCTION__ << " called without calling Init.";
  EXPECT_TRUE(initialization_succeeded) << __FUNCTION__
                                        << " called after a failed Init.";
  return allows_explicit_credentials_;
}

int HttpAuthHandlerMock::InitializeFromChallengeInternal(
    const HttpAuthChallengeTokenizer& challenge,
    const HttpResponseInfo& response_with_challenge,
    const CompletionCallback& callback) {
  EXPECT_FALSE(initialized_) << "Init already called once.";
  EXPECT_TRUE(challenge.SchemeIs(auth_scheme_))
      << "Mismatched scheme for challenge: " << challenge.challenge_text();
  EXPECT_TRUE(HttpAuth::IsValidNormalizedScheme(auth_scheme_))
      << "Invalid expected auth scheme.";

  base::StringPiece challenge_params(challenge.params_begin(),
                                     challenge.params_end());
  initialized_ = true;

  auth_token_ = auth_scheme_ + " auth_token";
  if (!challenge_params.empty()) {
    auth_token_ += ",";
    auth_token_.append(challenge.params_begin(), challenge.params_end());
  }

  if (init_async_) {
    callback_ = callback;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&HttpAuthHandlerMock::OnInitializeComplete,
                              weak_factory_.GetWeakPtr()));
    return ERR_IO_PENDING;
  }

  initialization_succeeded = (init_rv_ == OK);
  return init_rv_;
}

void HttpAuthHandlerMock::OnInitializeComplete() {
  ASSERT_FALSE(callback_.is_null());
  initialization_succeeded = (init_rv_ == OK);
  base::ResetAndReturn(&callback_).Run(init_rv_);
}

int HttpAuthHandlerMock::InitializeFromCacheEntryInternal(
    HttpAuthCache::Entry* cache_entry) {
  EXPECT_FALSE(initialized_) << "Init already called once.";
  EXPECT_EQ(auth_scheme_, cache_entry->scheme())
      << "Mismatched scheme for challenge: " << cache_entry->auth_challenge();
  EXPECT_TRUE(HttpAuth::IsValidNormalizedScheme(auth_scheme_))
      << "Invalid expected auth scheme.";

  HttpAuthChallengeTokenizer challenge(cache_entry->auth_challenge().begin(),
                                       cache_entry->auth_challenge().end());
  EXPECT_TRUE(challenge.SchemeIs(auth_scheme_))
      << "Mismatched scheme for challenge: " << challenge.challenge_text();

  auth_token_ = auth_scheme_ + " preemptive_auth_token";
  if (challenge.params_begin() != challenge.params_end()) {
    auth_token_ += ",";
    auth_token_.append(challenge.params_begin(), challenge.params_end());
  }

  initialized_ = true;
  initialization_succeeded = true;
  return OK;
}

int HttpAuthHandlerMock::GenerateAuthTokenImpl(
    const AuthCredentials* credentials,
    const HttpRequestInfo& request,
    const CompletionCallback& callback,
    std::string* auth_token) {
  EXPECT_TRUE(initialized_) << __FUNCTION__ << " called without calling Init.";
  EXPECT_TRUE(initialization_succeeded) << __FUNCTION__
                                        << " called after a failed Init.";
  first_round_ = false;
  request_url_ = request.url;

  if (credentials) {
    EXPECT_TRUE(AllowsExplicitCredentials()) << "Explicit credentials can only "
                                                "be specified if the handler "
                                                "supports it.";
  } else {
    EXPECT_TRUE(AllowsDefaultCredentials()) << "Credentials must be specified "
                                               "if the handler doesn't support "
                                               "default credentials.";
  }

  if (generate_async_) {
    EXPECT_TRUE(callback_.is_null()) << "GenerateAuthTokenImpl() called while "
                                        "a pending request is in progress";
    EXPECT_EQ(nullptr, generate_auth_token_buffer_)
        << "GenerateAuthTokenImpl() called before previous token was retrieved";
    callback_ = callback;
    generate_auth_token_buffer_ = auth_token;
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::Bind(&HttpAuthHandlerMock::OnGenerateAuthTokenComplete,
                              weak_factory_.GetWeakPtr()));
    return ERR_IO_PENDING;
  } else {
    if (generate_rv_ == OK)
      *auth_token = auth_token_;
    return generate_rv_;
  }
}

void HttpAuthHandlerMock::OnGenerateAuthTokenComplete() {
  EXPECT_TRUE(generate_async_);
  EXPECT_TRUE(!callback_.is_null());
  if (generate_rv_ == OK)
    *generate_auth_token_buffer_ = auth_token_;
  generate_auth_token_buffer_ = nullptr;
  base::ResetAndReturn(&callback_).Run(generate_rv_);
}

HttpAuthHandlerMock::Factory::Factory() {}

HttpAuthHandlerMock::Factory::~Factory() {
}

void HttpAuthHandlerMock::Factory::AddMockHandler(
    std::unique_ptr<HttpAuthHandler> handler,
    HttpAuthHandlerCreateReason reason) {
  if (reason == HttpAuthHandlerCreateReason::PREEMPTIVE)
    preemptive_handlers_.push_back(handler.Pass());
  else
    challenge_handlers_.push_back(handler.Pass());
}

bool HttpAuthHandlerMock::Factory::HaveAuthHandlers() const {
  return !challenge_handlers_.empty() || !preemptive_handlers_.empty();
}

std::unique_ptr<HttpAuthHandler> HttpAuthHandlerMock::Factory::GetNextAuthHandler(
    ScopedVector<HttpAuthHandler>* handler_list) {
  if (handler_list->empty())
    return std::unique_ptr<HttpAuthHandler>();
  std::unique_ptr<HttpAuthHandler> tmp_handler = std::move(handler_list->front());
  handler_list->erase(handler_list->begin());
  return tmp_handler;
}

std::unique_ptr<HttpAuthHandler>
HttpAuthHandlerMock::Factory::CreateAuthHandlerForScheme(
    const std::string& scheme) {
  std::unique_ptr<HttpAuthHandler> handler =
      GetNextAuthHandler(&challenge_handlers_);
  EXPECT_TRUE(!handler || handler->auth_scheme() == scheme)
      << "Next auth handler is for scheme " << handler->auth_scheme()
      << " while request scheme is " << scheme;
  return handler;
}

std::unique_ptr<HttpAuthHandler>
HttpAuthHandlerMock::Factory::CreateAndInitPreemptiveAuthHandler(
    HttpAuthCache::Entry* cache_entry,
    HttpAuth::Target target,
    const BoundNetLog& net_log) {
  std::unique_ptr<HttpAuthHandler> handler =
      GetNextAuthHandler(&preemptive_handlers_);
  if (handler)
    handler->InitializeFromCacheEntry(cache_entry, target, net_log);
  return handler;
}

}  // namespace net
