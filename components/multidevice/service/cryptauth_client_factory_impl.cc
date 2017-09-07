// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/cryptauth_client_factory_impl.h"

#include "components/cryptauth/cryptauth_client_impl.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/multidevice/service/cryptauth_token_fetcher_impl.h"
#include "net/url_request/url_request_context_getter.h"

namespace multidevice {

CryptAuthClientFactoryImpl::CryptAuthClientFactoryImpl(
    identity::mojom::IdentityManagerPtr* identity_manager,
    AccountInfo account_info,
    scoped_refptr<net::URLRequestContextGetter> url_request_context,
    const cryptauth::DeviceClassifier& device_classifier)
    : url_request_context_(url_request_context),
      identity_manager_(identity_manager),
      account_info_(account_info),
      device_classifier_(device_classifier) {}

CryptAuthClientFactoryImpl::~CryptAuthClientFactoryImpl() {}

std::unique_ptr<cryptauth::CryptAuthClient>
CryptAuthClientFactoryImpl::CreateInstance() {
  return base::MakeUnique<cryptauth::CryptAuthClientImpl>(
      base::WrapUnique(new cryptauth::CryptAuthApiCallFlow()),
      base::WrapUnique(new CryptAuthAccessTokenFetcherImpl(account_info_,
                                                           identity_manager_)),
      url_request_context_, device_classifier_);
}

}  // namespace multidevice