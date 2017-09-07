// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_CRYPTAUTH_ENROLLER_FACTORY_IMPL
#define COMPONENTS_MULTIDEVICE_CRYPTAUTH_ENROLLER_FACTORY_IMPL

#include "components/cryptauth/cryptauth_enroller.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/signin/core/browser/account_info.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"

namespace cryptauth {
class SecureMessageDelegateFactory;
}  // namespace cryptauth

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace multidevice {

class CryptAuthClientFactoryImpl;

class CryptAuthEnrollerFactoryImpl
    : public cryptauth::CryptAuthEnrollerFactory {
 public:
  CryptAuthEnrollerFactoryImpl(
      identity::mojom::IdentityManagerPtr* identity_manager,
      AccountInfo account_info,
      cryptauth::DeviceClassifier device_classifier,
      scoped_refptr<net::URLRequestContextGetter> url_request_context,
      cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory);

  ~CryptAuthEnrollerFactoryImpl() override;

  std::unique_ptr<cryptauth::CryptAuthEnroller> CreateInstance() override;

 private:
  scoped_refptr<net::URLRequestContextGetter> url_request_context_;
  cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory_;
  identity::mojom::IdentityManagerPtr* identity_manager_;
  AccountInfo account_info_;
  cryptauth::DeviceClassifier device_classifier_;
};

}  // namespace multidevice

#endif  // COMPONENTS_MULTIDEVICE_CRYPTAUTH_ENROLLER_FACTORY_IMPL