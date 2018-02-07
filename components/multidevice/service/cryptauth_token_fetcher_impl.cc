// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/cryptauth_token_fetcher_impl.h"

#include <set>

namespace multidevice {

namespace {

std::set<std::string> GetScopes() {
  std::set<std::string> scopes;
  scopes.insert("https://www.googleapis.com/auth/cryptauth");
  return scopes;
}

}  // namespace

CryptAuthAccessTokenFetcherImpl::CryptAuthAccessTokenFetcherImpl(
    const AccountInfo& account_info,
    identity::mojom::IdentityManager* identity_manager)
    : account_info_(account_info),
      identity_manager_(identity_manager),
      weak_ptr_factory_(this) {}

CryptAuthAccessTokenFetcherImpl::~CryptAuthAccessTokenFetcherImpl() {
  // If any callbacks are still pending when the object is deleted, run them
  // now, passing an empty string to indicate failure. This ensures that no
  // callbacks are left hanging forever.
  for (auto& map_entry : request_id_to_callback_map_)
    map_entry.second.Run(std::string());
}

void CryptAuthAccessTokenFetcherImpl::FetchAccessToken(
    const AccessTokenCallback& callback) {
  uint32_t request_id = next_request_id_++;
  request_id_to_callback_map_[request_id] = callback;
  identity_manager_->GetAccessToken(
      account_info_.account_id, GetScopes(), account_info_.given_name,
      base::Bind(&CryptAuthAccessTokenFetcherImpl::OnAccessTokenFetched,
                 weak_ptr_factory_.GetWeakPtr(), callback, request_id));
}

void CryptAuthAccessTokenFetcherImpl::OnAccessTokenFetched(
    const AccessTokenCallback& callback,
    uint32_t request_id,
    const base::Optional<std::string>& access_token,
    base::Time expiration_time,
    const GoogleServiceAuthError& error) {
  request_id_to_callback_map_.erase(request_id);
  callback.Run(access_token.has_value() ? access_token.value() : std::string());
}

}  // namespace multidevice
