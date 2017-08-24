// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_service_ssl_config_service.h"

namespace content {

NetworkServiceSSLConfigService::NetworkServiceSSLConfigService(
    base::Optional<net::SSLConfig> initial_config,
    base::Optional<mojom::SSLConfigClientRequest> ssl_config_client_request)
    : binding_(this),
      ssl_config_(initial_config ? std::move(*initial_config)
                                 : net::SSLConfig()) {
  if (ssl_config_client_request)
    binding_.Bind(std::move(*ssl_config_client_request));
}

void NetworkServiceSSLConfigService::UpdateSSLConfig(
    const net::SSLConfig& ssl_config) {
  net::SSLConfig old_config = ssl_config_;
  ssl_config_ = ssl_config;
  ProcessConfigUpdate(old_config, ssl_config_);
}

void NetworkServiceSSLConfigService::GetSSLConfig(net::SSLConfig* ssl_config) {
  *ssl_config = ssl_config_;
}

NetworkServiceSSLConfigService::~NetworkServiceSSLConfigService() {}

}  // namespace content
