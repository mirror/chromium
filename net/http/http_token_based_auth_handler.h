// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "net/http/http_auth.h"
#include "base/memory/scoped_ptr.h"
#include "net/base/completion_callback.h"
#include "net/base/net_errors.h"
#include "net/http/http_auth_handler.h"
#include "net/http/http_auth_handler_factory.h"

namespace net {

class AuthCredentials;
class AuthLibrary;
class HostResolver;
class URLSecurityManager;

class HttpTokenBasedAuthHandler final : public HttpAuthHandler {
 public:
  // An abstraction for an HTTP authentication system that uses:
  //   * Multi-round authentication handshakes over a single connection.
  //   * At each round-trip, takes a single binary blob as input and spits out
  //     another one in response.
  //   * Initial round-trip is performed with no incoming blob.
  //
  // Astute observers will notice that this is basically describing NTLM and
  // Negotiate authentication. That would be correct.
  //
  // Instances of HttpAuthSystem are only alive long enough to fulfill a single
  // authentication handshake.
  class AuthSystem {
   public:
    // TODO(asanka): Make this asynchronous when challenge handling becomes
    // asynchronous.
    virtual Error Initialize(const AuthLibrary* library) = 0;

    virtual bool AllowsExplicitCredentials() const = 0;

    virtual bool AllowsDefaultCredentials() const = 0;

    virtual bool IsHandshakeInProgress() const = 0;

    enum AuthTokenOptions {
      TOKEN_DEFAULT,
      TOKEN_DELEGATE
    };
    virtual Error GenerateAuthToken(const AuthCredentials* credentials,
                                    const std::string& canonical_target,
                                    const std::string& inbound_token,
                                    const CompletionCallback& callback,
                                    AuthTokenOptions token_options,
                                    std::string* auth_token) = 0;
  };

  HttpTokenBasedAuthHandler(scoped_ptr<AuthSystem> auth_system,
                            URLSecurityManager* url_security_manager);
  ~HttpTokenBasedAuthHandler() override;

  // HttpAuthHandler
  bool Init(HttpAuthChallengeTokenizer* challenge) override;
  HttpAuth::AuthorizationResult HandleAnotherChallenge(
      HttpAuthChallengeTokenizer* challenge) override;
  bool NeedsIdentity() override;
  bool AllowsDefaultCredentials() override;
  bool AllowsExplicitCredentials() override;
  int GenerateAuthTokenImpl(const AuthCredentials* credentials,
                            const HttpRequestInfo* request,
                            const CompletionCallback& callback,
                            std::string* auth_token) override;
 private:
  enum State {
    STATE_NONE,
    STATE_RESOLVE_CANONICAL_NAME,
    STATE_RESOLVE_CANONICAL_NAME_COMPLETE,
    STATE_GENERATE_AUTH_TOKEN,
    STATE_GENERATE_AUTH_TOKEN_COMPLETE
  };

  Error DoLoop(Error result);
  void OnIOComplete(int rv);
  void DoCallback(int rv);

  Error DoResolveCanonicalName();
  Error DoResolveCanonicalNameComplete(Error result);
  Error DoGenerateAuthToken();
  Error DoGenerateAuthTokenComplete(Error result);

  scoped_ptr<AuthSystem> auth_system_;
  std::string inbound_token_;
  std::string outbound_token_;
  URLSecurityManager* url_security_manager_ = nullptr;
  State next_state_ = STATE_NONE;
  CompletionCallback callback_;
  CompletionCallback io_completion_callback_;
  bool disable_cname_lookup_ = false;
};

}  // namespace net
