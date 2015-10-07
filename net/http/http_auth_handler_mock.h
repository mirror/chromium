// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_AUTH_HANDLER_MOCK_H_
#define NET_HTTP_HTTP_AUTH_HANDLER_MOCK_H_

#include <memory>
#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "net/http/http_auth.h"
#include "net/http/http_auth_handler.h"
#include "net/http/http_auth_handler_factory.h"
#include "url/gurl.h"

namespace net {

class HostResolver;

enum class HttpAuthHandlerCreateReason { PREEMPTIVE, CHALLENGE };

// MockAuthHandler is used in tests to reliably trigger edge cases.
class HttpAuthHandlerMock : public HttpAuthHandler {
 public:
  // The Factory class returns handlers in the order they were added via
  // AddMockHandler.
  class Factory : public HttpAuthHandlerFactory {
   public:
    Factory();
    ~Factory() override;

    void AddMockHandler(std::unique_ptr<HttpAuthHandler> handler,
                        HttpAuthHandlerCreateReason reason);

    bool HaveAuthHandlers() const;

    // HttpAuthHandlerFactory:
    std::unique_ptr<HttpAuthHandler> CreateAuthHandlerForScheme(
        const std::string& scheme) override;
    std::unique_ptr<HttpAuthHandler> CreateAndInitPreemptiveAuthHandler(
        HttpAuthCache::Entry* cache_entry,
        const HttpAuthChallengeTokenizer& tokenizer,
        HttpAuth::Target target,
        const BoundNetLog& net_log) override;

   private:
    std::unique_ptr<HttpAuthHandler> GetNextAuthHandler(
        std::vector<std::unique_ptr<HttpAuthHandler>>* handler_list);
    std::vector<std::unique_ptr<HttpAuthHandler>> challenge_handlers_;
    std::vector<std::unique_ptr<HttpAuthHandler>> preemptive_handlers_;
  };

  HttpAuthHandlerMock();

  ~HttpAuthHandlerMock() override;

  void SetGenerateExpectation(bool async, int rv);

  void set_expected_auth_scheme(const std::string& scheme) {
    auth_scheme_ = scheme;
  }

  void set_expected_auth_target(HttpAuth::Target target) {
    expected_auth_target_ = target;
  }

  void set_expect_multiple_challenges(bool expect_multiple_challenges) {
    expect_multiple_challenges_ = expect_multiple_challenges;
  }

  void set_allows_default_credentials(bool allows_default_credentials) {
    allows_default_credentials_ = allows_default_credentials;
  }

  void set_allows_explicit_credentials(bool allows_explicit_credentials) {
    allows_explicit_credentials_ = allows_explicit_credentials;
  }

  void set_auth_token(const std::string& auth_token) {
    auth_token_ = auth_token;
  }

  const GURL& request_url() const {
    return request_url_;
  }

  // HttpAuthHandler:
  HttpAuth::AuthorizationResult HandleAnotherChallenge(
      const HttpAuthChallengeTokenizer& challenge) override;
  bool NeedsIdentity() override;
  bool AllowsDefaultCredentials() override;
  bool AllowsExplicitCredentials() override;

 protected:
  int Init(const HttpAuthChallengeTokenizer& challenge,
           const SSLInfo& ssl_info) override;

  int GenerateAuthTokenImpl(const AuthCredentials* credentials,
                            const HttpRequestInfo& request,
                            const CompletionCallback& callback,
                            std::string* auth_token) override;

 private:
  void OnGenerateAuthToken();

  CompletionCallback callback_;
  bool generate_async_ = false;
  int generate_rv_ = 0;
  std::string auth_token_;
  std::string* generate_auth_token_buffer_ = nullptr;
  bool first_round_ = true;
  bool allows_default_credentials_ = false;
  bool allows_explicit_credentials_ = true;
  bool expect_multiple_challenges_ = false;
  HttpAuth::Target expected_auth_target_ = HttpAuth::AUTH_SERVER;
  GURL request_url_;
  base::WeakPtrFactory<HttpAuthHandlerMock> weak_factory_;
};

}  // namespace net

#endif  // NET_HTTP_HTTP_AUTH_HANDLER_MOCK_H_
