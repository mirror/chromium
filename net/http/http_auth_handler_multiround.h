// Copyright 2015 The Chromium Authors. All rights reserved.  Use of this source
// code is governed by a BSD-style license that can be found in the LICENSE
// file.

#ifndef NET_HTTP_HTTP_AUTH_HANDLER_MULTIROUND_H_
#define NET_HTTP_HTTP_AUTH_HANDLER_MULTIROUND_H_

#include "net/base/address_list.h"
#include "base/memory/scoped_ptr.h"
#include "net/base/completion_callback.h"
#include "net/base/net_export.h"
#include "net/http/http_auth_handler.h"

namespace net {

class URLSecurityManager;
class HostResolver;
class SingleRequestHostResolver;

class NET_EXPORT_PRIVATE HttpAuthHandlerMultiRound : public HttpAuthHandler {
 public:
  virtual ~HttpAuthHandlerMultiRound();

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
  HttpAuth::AuthorizationResult HandleAnotherChallenge(
      const HttpAuthChallengeTokenizer& challenge) override;
  bool NeedsIdentity() override;
  bool AllowsDefaultCredentials() override;
  bool AllowsExplicitCredentials() override;

 protected:
  class NET_EXPORT_PRIVATE Platform {
   public:
    virtual int InitializeFromChallenge(
        const URLSecurityManager* url_security_manager,
        const HttpResponseInfo& response_with_challenge,
        const CompletionCallback& callback) = 0;

    virtual int GenerateInitialAuthToken(
        const URLSecurityManager* url_security_manager,
        const AuthCredentials* credentials,
        const std::string& service_principal_name,
        const CompletionCallback& callback,
        std::string* auth_token) = 0;

    virtual int GenerateContinuationToken(
        const std::string& inbound_token,
        const CompletionCallback& callback,
        std::string* auth_token) = 0;
  };

  enum class PrincipalHostNamePolicy {
    USE_AS_GIVEN,
    CANONICALIZE
  };

  enum class PrincipalPortPolicy {
    INCLUDE_PORT_IN_PRINCIPAL,
    IGNORE
  };

  HttpAuthHandlerMultiRound(const std::string& scheme,
                            scoped_ptr<Platform> platform,
                            PrincipalHostNamePolicy host_name_policy,
                            PrincipalPortPolicy port_policy,
                            const URLSecurityManager* url_security_manager,
                            HostResolver* host_resolver);

 private:
  enum State {
    STATE_INITIALIZE,
    STATE_INITIALIZE_COMPLETE,
    STATE_RESOLVE_CANONICAL_NAME,
    STATE_RESOLVE_CANONICAL_NAME_COMPLETE,
    STATE_GENERATE_AUTH_TOKEN,
    STATE_GENERATE_AUTH_TOKEN_COMPLETE,
    STATE_NONE
  };

  void OnIOComplete(int result);
  int DoLoop(int result);

  int DoInitialize();
  int DoInitializeComplete(int result);
  int DoResolveCanonicalName();
  int DoResolveCanonicalNameComplete(int result);
  int DoGenerateAuthToken();
  int DoGenerateAuthTokenComplete(int result);

  std::string CreateSPN(const AddressList& address_list,
                        const GURL& origin);

  CompletionCallback io_callback_;
  CompletionCallback callback_;
  State next_state_;

  // Members which are needed for DNS lookup + SPN.
  AddressList address_list_;
  scoped_ptr<SingleRequestHostResolver> single_resolve_;

  scoped_ptr<Platform> platform_;
  PrincipalHostNamePolicy host_name_policy_;
  PrincipalPortPolicy port_policy_;
  const URLSecurityManager* url_security_manager_;
  std::string service_principal_name_;
  HostResolver* host_resolver_;
};

} // namespace net

#endif // NET_HTTP_HTTP_AUTH_HANDLER_MULTIROUND_H_
