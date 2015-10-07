// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_AUTH_HANDLER_BASIC_H_
#define NET_HTTP_HTTP_AUTH_HANDLER_BASIC_H_

#include <string>

#include "net/base/net_export.h"
#include "net/http/http_auth_handler.h"
#include "net/http/http_auth_handler_factory.h"

namespace net {

// Code for handling http basic authentication.
class NET_EXPORT_PRIVATE HttpAuthHandlerBasic : public HttpAuthHandler {
 public:
  class NET_EXPORT_PRIVATE Factory : public HttpAuthHandlerFactory {
   public:
    Factory();
    ~Factory() override;

    // HttpAuthHandlerFactory
    std::unique_ptr<HttpAuthHandler> CreateAuthHandlerForScheme(
        const std::string& scheme) override;
    std::unique_ptr<HttpAuthHandler> CreateAndInitPreemptiveAuthHandler(
        HttpAuthCache::Entry* cache_entry,
        HttpAuth::Target target,
        const BoundNetLog& net_log) override;
  };

  ~HttpAuthHandlerBasic() override {}

  // HttpAuthHandler
  HttpAuth::AuthorizationResult HandleAnotherChallenge(
      const HttpAuthChallengeTokenizer& challenge) override;

 protected:
  // HttpAuthHandler
  int InitializeFromChallengeInternal(
      const HttpAuthChallengeTokenizer& challenge,
      const HttpResponseInfo& response_with_challenge,
      const CompletionCallback& callback) override;
  int InitializeFromCacheEntryInternal(
      HttpAuthCache::Entry* cache_entry) override;
  int GenerateAuthTokenImpl(const AuthCredentials* credentials,
                            const HttpRequestInfo& request,
                            const CompletionCallback& callback,
                            std::string* auth_token) override;

 private:
  HttpAuthHandlerBasic();
  int ParseChallenge(const HttpAuthChallengeTokenizer& challenge);
};

}  // namespace net

#endif  // NET_HTTP_HTTP_AUTH_HANDLER_BASIC_H_
