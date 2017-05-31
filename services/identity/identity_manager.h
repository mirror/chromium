// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_IDENTITY_IDENTITY_MANAGER_H_
#define SERVICES_IDENTITY_IDENTITY_MANAGER_H_

#include "components/signin/core/account_id/account_id.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "services/identity/public/cpp/scope_set.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"

class SigninManagerBase;

namespace identity {

class IdentityManager : public mojom::IdentityManager,
                        public OAuth2TokenService::Consumer {
 public:
  static void Create(mojom::IdentityManagerRequest request,
                     SigninManagerBase* signin_manager,
                     ProfileOAuth2TokenService* token_service);

  IdentityManager(SigninManagerBase* signin_manager,
                  ProfileOAuth2TokenService* token_service);
  ~IdentityManager() override;

 private:
  // Holds the information needed for the duration between receiving a consumer
  // request to get an access token and receiving the callback from the
  // subsequent request that this instance makes to OAuth2TokenService.
  struct AccessTokenRequest {
    AccessTokenRequest();
    AccessTokenRequest(AccessTokenRequest&& other);
    ~AccessTokenRequest();
    AccessTokenRequest& operator=(AccessTokenRequest&& other);

    std::unique_ptr<OAuth2TokenService::Request> token_service_request;
    GetAccessTokenCallback consumer_callback;
  };
  using AccessTokenRequests =
      std::map<const OAuth2TokenService::Request*, AccessTokenRequest>;

  // mojom::IdentityManager:
  void GetPrimaryAccountId(GetPrimaryAccountIdCallback callback) override;
  void GetAccessToken(const std::string& account_id,
                      const ScopeSet& scopes,
                      GetAccessTokenCallback callback) override;

  // OAuth2TokenService::Consumer:
  void OnGetTokenSuccess(const OAuth2TokenService::Request* request,
                         const std::string& access_token,
                         const base::Time& expiration_time) override;
  void OnGetTokenFailure(const OAuth2TokenService::Request* request,
                         const GoogleServiceAuthError& error) override;

  // Completes the pending access token request by calling back the consumer.
  void OnAccessTokenRequestCompleted(
      const OAuth2TokenService::Request* request,
      const base::Optional<std::string>& access_token,
      const base::Time& expiration_time,
      const GoogleServiceAuthError& error);

  SigninManagerBase* signin_manager_;
  ProfileOAuth2TokenService* token_service_;

  // The set of pending requests for access tokens.
  AccessTokenRequests access_token_requests_;
};

}  // namespace identity

#endif  // SERVICES_IDENTITY_IDENTITY_MANAGER_H_
