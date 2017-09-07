// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/cryptauth_token_fetcher_impl.h"

#include <set>

#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device_provider.h"
#include "net/url_request/url_request_context_getter.h"

namespace {
std::set<std::string> GetScopes() {
  std::set<std::string> scopes;
  scopes.insert("https://www.googleapis.com/auth/cryptauth");
  return scopes;
}
}  // namespace

namespace multidevice {

CryptAuthAccessTokenFetcherImpl::CryptAuthAccessTokenFetcherImpl(
    AccountInfo account_info,
    identity::mojom::IdentityManagerPtr* identity_manager)
    : account_info_(account_info),
      identity_manager_(identity_manager),
      weak_ptr_factory_(this) {}

CryptAuthAccessTokenFetcherImpl::~CryptAuthAccessTokenFetcherImpl() {}

// cryptauth::CryptAuthAccessTokenFetcher:
void CryptAuthAccessTokenFetcherImpl::FetchAccessToken(
    const AccessTokenCallback& callback) {
  (*identity_manager_)
      ->GetAccessToken(
          account_info_.account_id, GetScopes(), account_info_.given_name,
          base::Bind(&CryptAuthAccessTokenFetcherImpl::OnAccessTokenFetched,
                     weak_ptr_factory_.GetWeakPtr(), callback));
}

void CryptAuthAccessTokenFetcherImpl::OnAccessTokenFetched(
    AccessTokenCallback callback,
    const base::Optional<std::string>& access_token,
    base::Time expiration_time,
    const ::GoogleServiceAuthError& error) {
  callback.Run(access_token.has_value() ? access_token.value() : std::string());
}

}  // namespace multidevice