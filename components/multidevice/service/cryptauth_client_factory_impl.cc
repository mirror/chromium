// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/cryptauth_client_factory_impl.h"

#include "components/cryptauth/cryptauth_client_impl.h"
#include "components/multidevice/service/cryptauth_token_fetcher_impl.h"
#include "net/url_request/url_request_context_getter.h"

namespace multidevice {

CryptAuthClientFactoryImpl::CryptAuthClientFactoryImpl(
    identity::mojom::IdentityManager* identity_manager,
    scoped_refptr<net::URLRequestContextGetter> url_request_context,
    const AccountInfo& account_info,
    const cryptauth::DeviceClassifier& device_classifier)
    : identity_manager_(identity_manager),
      url_request_context_(std::move(url_request_context)),
      account_info_(account_info),
      device_classifier_(device_classifier) {}

CryptAuthClientFactoryImpl::~CryptAuthClientFactoryImpl() = default;

std::unique_ptr<cryptauth::CryptAuthClient>
CryptAuthClientFactoryImpl::CreateInstance() {
  return base::MakeUnique<cryptauth::CryptAuthClientImpl>(
      base::MakeUnique<cryptauth::CryptAuthApiCallFlow>(),
      base::WrapUnique(new CryptAuthAccessTokenFetcherImpl(account_info_,
                                                           identity_manager_)),
      url_request_context_, device_classifier_);
}

}  // namespace multidevice
