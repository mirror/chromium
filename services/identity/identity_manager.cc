// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/identity/identity_manager.h"

#include "base/time/time.h"
#include "components/signin/core/browser/account_info.h"
#include "components/signin/core/browser/signin_manager_base.h"
#include "google_apis/gaia/google_service_auth_error.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace identity {

IdentityManager::AccessTokenRequest::AccessTokenRequest() = default;

IdentityManager::AccessTokenRequest::AccessTokenRequest(
    IdentityManager::AccessTokenRequest&& other) {
  *this = std::move(other);
}

IdentityManager::AccessTokenRequest::~AccessTokenRequest() = default;

IdentityManager::AccessTokenRequest& IdentityManager::AccessTokenRequest::
operator=(IdentityManager::AccessTokenRequest&& other) {
  if (this != &other) {
    token_service_request = std::move(other.token_service_request);
    consumer_callback = other.consumer_callback;
  }
  return *this;
}

// static
void IdentityManager::Create(mojom::IdentityManagerRequest request,
                             SigninManagerBase* signin_manager,
                             ProfileOAuth2TokenService* token_service) {
  mojo::MakeStrongBinding(
      base::MakeUnique<IdentityManager>(signin_manager, token_service),
      std::move(request));
}

IdentityManager::IdentityManager(SigninManagerBase* signin_manager,
                                 ProfileOAuth2TokenService* token_service)
    : OAuth2TokenService::Consumer("identity_service"),
      signin_manager_(signin_manager),
      token_service_(token_service) {}

IdentityManager::~IdentityManager() {}

void IdentityManager::GetPrimaryAccountId(
    const GetPrimaryAccountIdCallback& callback) {
  AccountId account_id = EmptyAccountId();

  if (signin_manager_->IsAuthenticated()) {
    AccountInfo account_info = signin_manager_->GetAuthenticatedAccountInfo();
    account_id =
        AccountId::FromUserEmailGaiaId(account_info.email, account_info.gaia);
  }

  callback.Run(account_id);
}

void IdentityManager::GetAccessToken(const std::string& account_id,
                                     const ScopeSet& scopes,
                                     const GetAccessTokenCallback& callback) {
  std::unique_ptr<OAuth2TokenService::Request> token_service_request =
      token_service_->StartRequest(account_id, scopes, this);
  OAuth2TokenService::Request* raw_token_service_request =
      token_service_request.get();

  AccessTokenRequest access_token_request;
  access_token_request.token_service_request = std::move(token_service_request);
  access_token_request.consumer_callback = callback;

  access_token_requests_[raw_token_service_request] =
      std::move(access_token_request);
}

void IdentityManager::OnGetTokenSuccess(
    const OAuth2TokenService::Request* request,
    const std::string& access_token,
    const base::Time& expiration_time) {
  OnAccessTokenRequestCompleted(
      request, base::Optional<std::string>(access_token), expiration_time,
      GoogleServiceAuthError::AuthErrorNone());
}

void IdentityManager::OnGetTokenFailure(
    const OAuth2TokenService::Request* request,
    const GoogleServiceAuthError& error) {
  OnAccessTokenRequestCompleted(request, base::nullopt, base::Time(), error);
}

void IdentityManager::OnAccessTokenRequestCompleted(
    const OAuth2TokenService::Request* request,
    const base::Optional<std::string>& access_token,
    const base::Time& expiration_time,
    const GoogleServiceAuthError& error) {
  auto it = access_token_requests_.find(request);
  DCHECK(it != access_token_requests_.end());
  GetAccessTokenCallback consumer_callback = it->second.consumer_callback;

  access_token_requests_.erase(request);
  consumer_callback.Run(access_token, expiration_time, error);
}

}  // namespace identity
