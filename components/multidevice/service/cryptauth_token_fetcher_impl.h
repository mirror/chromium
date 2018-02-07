// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_CRYPTAUTH_ACCESS_TOKEN_FETCHER_IMPL
#define COMPONENTS_MULTIDEVICE_CRYPTAUTH_ACCESS_TOKEN_FETCHER_IMPL

#include <unordered_map>

#include "components/cryptauth/cryptauth_access_token_fetcher.h"
#include "components/signin/core/browser/account_info.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"

namespace multidevice {

// CryptAuthAccessTokenFetcher implementation which utilizes IdentityManager.
class CryptAuthAccessTokenFetcherImpl
    : public cryptauth::CryptAuthAccessTokenFetcher {
 public:
  CryptAuthAccessTokenFetcherImpl(
      const AccountInfo& account_info,
      identity::mojom::IdentityManager* identity_manager);

  ~CryptAuthAccessTokenFetcherImpl() override;

  // cryptauth::CryptAuthAccessTokenFetcher:
  void FetchAccessToken(const AccessTokenCallback& callback) override;

 private:
  void OnAccessTokenFetched(const AccessTokenCallback& callback,
                            uint32_t request_id,
                            const base::Optional<std::string>& access_token,
                            base::Time expiration_time,
                            const ::GoogleServiceAuthError& error);

  const AccountInfo account_info_;
  identity::mojom::IdentityManager* identity_manager_;

  uint32_t next_request_id_ = 0;
  std::unordered_map<uint32_t, AccessTokenCallback> request_id_to_callback_map_;

  base::WeakPtrFactory<CryptAuthAccessTokenFetcherImpl> weak_ptr_factory_;
};

}  // namespace multidevice

#endif  // COMPONENTS_MULTIDEVICE_CRYPTAUTH_ACCESS_TOKEN_FETCHER_IMPL
