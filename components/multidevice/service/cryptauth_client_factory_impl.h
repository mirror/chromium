// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_CRYPTAUTH_CLIENT_FACTORY_IMPL
#define COMPONENTS_MULTIDEVICE_CRYPTAUTH_CLIENT_FACTORY_IMPL

#include "components/cryptauth/cryptauth_client.h"
#include "components/signin/core/browser/account_info.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"

namespace cryptauth {
class DeviceClassifier;
}  // namespace cryptauth

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace multidevice {

class CryptAuthClientFactoryImpl : public cryptauth::CryptAuthClientFactory {
 public:
  CryptAuthClientFactoryImpl(
      identity::mojom::IdentityManagerPtr* identity_manager,
      AccountInfo account_info,
      scoped_refptr<net::URLRequestContextGetter> url_request_context,
      const cryptauth::DeviceClassifier& device_classifier);

  ~CryptAuthClientFactoryImpl() override;

  // cryptauth::CryptAuthClientFactory:
  std::unique_ptr<cryptauth::CryptAuthClient> CreateInstance() override;

 private:
  scoped_refptr<net::URLRequestContextGetter> url_request_context_;
  identity::mojom::IdentityManagerPtr* identity_manager_;
  AccountInfo account_info_;
  const cryptauth::DeviceClassifier& device_classifier_;
};

}  // namespace multidevice

#endif  // COMPONENTS_MULTIDEVICE_CRYPTAUTH_CLIENT_FACTORY_IMPL