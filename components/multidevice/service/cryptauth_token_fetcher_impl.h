// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_CRYPTAUTH_ACCESS_TOKEN_FETCHER_IMPL
#define COMPONENTS_MULTIDEVICE_CRYPTAUTH_ACCESS_TOKEN_FETCHER_IMPL

#include "components/cryptauth/cryptauth_access_token_fetcher.h"
#include "components/signin/core/browser/account_info.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"

namespace cryptauth {
class SecureMessageDelegateFactory;
}  // namespace cryptauth

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace multidevice {

class CryptAuthAccessTokenFetcherImpl
    : public cryptauth::CryptAuthAccessTokenFetcher {
 public:
  CryptAuthAccessTokenFetcherImpl(
      AccountInfo account_info,
      identity::mojom::IdentityManagerPtr* identity_manager);

  ~CryptAuthAccessTokenFetcherImpl() override;

  // cryptauth::CryptAuthAccessTokenFetcher:
  void FetchAccessToken(const AccessTokenCallback& callback) override;

  void OnAccessTokenFetched(AccessTokenCallback callback,
                            const base::Optional<std::string>& access_token,
                            base::Time expiration_time,
                            const ::GoogleServiceAuthError& error);

 private:
  AccountInfo account_info_;
  identity::mojom::IdentityManagerPtr* identity_manager_;
  base::WeakPtrFactory<CryptAuthAccessTokenFetcherImpl> weak_ptr_factory_;
};

}  // namespace multidevice

#endif  // COMPONENTS_MULTIDEVICE_CRYPTAUTH_ACCESS_TOKEN_FETCHER_IMPL