// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/cryptauth_enroller_factory_impl.h"

#include "components/cryptauth/cryptauth_enroller_impl.h"
#include "components/cryptauth/remote_device_provider.h"
#include "components/cryptauth/secure_message_delegate.h"
#include "components/multidevice/service/cryptauth_client_factory_impl.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"

namespace multidevice {

CryptAuthEnrollerFactoryImpl::CryptAuthEnrollerFactoryImpl(
    identity::mojom::IdentityManagerPtr* identity_manager,
    AccountInfo account_info,
    cryptauth::DeviceClassifier device_classifier,
    scoped_refptr<net::URLRequestContextGetter> url_request_context,
    cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory)
    : url_request_context_(url_request_context),
      secure_message_delegate_factory_(secure_message_delegate_factory),
      identity_manager_(identity_manager),
      account_info_(account_info),
      device_classifier_() {}

CryptAuthEnrollerFactoryImpl::~CryptAuthEnrollerFactoryImpl() {}

std::unique_ptr<cryptauth::CryptAuthEnroller>
CryptAuthEnrollerFactoryImpl::CreateInstance() {
  return base::MakeUnique<cryptauth::CryptAuthEnrollerImpl>(
      base::MakeUnique<CryptAuthClientFactoryImpl>(
          identity_manager_, account_info_, url_request_context_,
          device_classifier_),
      secure_message_delegate_factory_->CreateSecureMessageDelegate());
}

}  // namespace multidevice
