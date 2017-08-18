// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_NETWORK_SERVICE_SSL_CONFIG_SERVICE_H_
#define CONTENT_NETWORK_NETWORK_SERVICE_SSL_CONFIG_SERVICE_H_

#include "base/optional.h"
#include "content/common/content_export.h"
#include "content/public/common/ssl_config.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/ssl/ssl_config.h"
#include "net/ssl/ssl_config_service.h"

namespace content {

// An SSLConfigClient that serves as a net::SSLConfigService, listening to
// SSLConfig changes on a Mojo pipe, and providing access to the updated config.
// TODO(mmenke): If other network embedders are all switch to using the network
// service, this can be  merged with SSLConfigServicePref.
class CONTENT_EXPORT NetworkServiceSSLConfigService
    : public mojom::SSLConfigClient,
      public net::SSLConfigService {
 public:
  // If |initial_config| is not provided, just uses the default configuration.
  // If |ssl_config_client_request| is not provided, just sticks with the
  // initial configuration.
  NetworkServiceSSLConfigService(
      base::Optional<net::SSLConfig> initial_config,
      base::Optional<mojom::SSLConfigClientRequest> ssl_config_client_request);

  // mojom::SSLConfigClient implementation:
  void UpdateSSLConfig(const net::SSLConfig& ssl_config) override;

  // net::SSLConfigClient implementation.
  void GetSSLConfig(net::SSLConfig* ssl_config) override;

 private:
  ~NetworkServiceSSLConfigService() override;

  mojo::Binding<mojom::SSLConfigClient> binding_;

  net::SSLConfig ssl_config_;

  DISALLOW_COPY_AND_ASSIGN(NetworkServiceSSLConfigService);
};

}  // namespace content

#endif  // CONTENT_NETWORK_NETWORK_SERVICE_SSL_CONFIG_SERVICE_H_
