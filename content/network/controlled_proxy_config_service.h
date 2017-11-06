// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_NETWORK_CONTROLLED_PROXY_CONFIG_SERVICE_H_
#define CONTENT_NETWORK_CONTROLLED_PROXY_CONFIG_SERVICE_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "base/optional.h"
#include "content/public/common/network_service.mojom.h"
#include "content/public/common/proxy_config.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "net/proxy/proxy_config.h"
#include "net/proxy/proxy_config_service.h"

namespace net {
class ProxyConfig;
}

namespace content {

class CONTENT_EXPORT ControlledProxyConfigService
    : public net::ProxyConfigService,
      public mojom::ProxyConfigClient {
 public:
  explicit ControlledProxyConfigService(
      mojom::ProxyConfigPollerClientPtr proxy_poller_client,
      base::Optional<net::ProxyConfig> initial_proxy_config,
      mojom::ProxyConfigClientAssociatedRequest proxy_config_client_request);
  ~ControlledProxyConfigService() override;

  // net::ProxyConfigService implementation:
  void AddObserver(Observer* observer) override;
  void RemoveObserver(Observer* observer) override;
  ConfigAvailability GetLatestProxyConfig(net::ProxyConfig* config) override;
  void OnLazyPoll() override;

 private:
  // mojom::ProxyConfigClient implementation:
  void OnProxyConfigUpdated(const net::ProxyConfig& proxy_config) override;

  mojom::ProxyConfigPollerClientPtr proxy_poller_client_;

  net::ProxyConfig config_;
  bool config_pending_ = true;

  mojo::AssociatedBinding<mojom::ProxyConfigClient> binding_;

  base::ObserverList<Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(ControlledProxyConfigService);
};

}  // namespace content

#endif  // CONTENT_NETWORK_CONTROLLED_PROXY_CONFIG_SERVICE_H_
