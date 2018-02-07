// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/cryptauth_enroller_factory_impl.h"

#include "components/cryptauth/cryptauth_enroller_impl.h"
#include "components/multidevice/service/cryptauth_client_factory_impl.h"
#include "net/url_request/url_request_context_getter.h"

namespace multidevice {

CryptAuthEnrollerFactoryImpl::CryptAuthEnrollerFactoryImpl(
    identity::mojom::IdentityManager* identity_manager,
    cryptauth::SecureMessageDelegate::Factory* secure_message_delegate_factory,
    scoped_refptr<net::URLRequestContextGetter> url_request_context,
    const AccountInfo& account_info,
    const cryptauth::DeviceClassifier& device_classifier)
    : identity_manager_(identity_manager),
      secure_message_delegate_factory_(secure_message_delegate_factory),
      url_request_context_(std::move(url_request_context)),
      account_info_(account_info),
      device_classifier_(device_classifier) {}

CryptAuthEnrollerFactoryImpl::~CryptAuthEnrollerFactoryImpl() {}

std::unique_ptr<cryptauth::CryptAuthEnroller>
CryptAuthEnrollerFactoryImpl::CreateInstance() {
  return base::MakeUnique<cryptauth::CryptAuthEnrollerImpl>(
      base::MakeUnique<CryptAuthClientFactoryImpl>(
          identity_manager_, url_request_context_, account_info_,
          device_classifier_),
      secure_message_delegate_factory_->CreateSecureMessageDelegate());
}

}  // namespace multidevice
