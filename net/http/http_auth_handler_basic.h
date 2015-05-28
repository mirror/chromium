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

    int CreateAuthHandler(HttpAuthChallengeTokenizer* challenge,
                          HttpAuth::Target target,
                          const SSLInfo& ssl_info,
                          const GURL& origin,
                          CreateReason reason,
                          int digest_nonce_count,
                          const NetLogWithSource& net_log,
                          std::unique_ptr<HttpAuthHandler>* handler) override;
  };

  HttpAuth::AuthorizationResult HandleAnotherChallenge(
      HttpAuthChallengeTokenizer* challenge) override;

 protected:
  int GenerateAuthToken(const AuthCredentials* credentials,
                        const HttpRequestInfo* request,
                        const CompletionCallback& callback,
                        std::string* auth_token) override;

 private:
  HttpAuthHandlerBasic(const std::string& realm,
                       const std::string& challenge,
                       const GURL& origin,
                       HttpAuth::Target target,
                       const BoundNetLog& net_log);
  ~HttpAuthHandlerBasic() override {}

  bool ParseChallenge(HttpAuthChallengeTokenizer* challenge);

  static const char kScheme[]; // Name of scheme: "basic"
  static const int kScore; // Priority score: 1
};

}  // namespace net

#endif  // NET_HTTP_HTTP_AUTH_HANDLER_BASIC_H_
