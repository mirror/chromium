// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/controlled_proxy_config_service.h"

namespace content {

ControlledProxyConfigService::ControlledProxyConfigService(
    mojom::ProxyConfigLazyPollerPtr proxy_poller)
    : proxy_poller_(std::move(proxy_poller)) {}

ControlledProxyConfigService::~ControlledProxyConfigService() {}

void ControlledProxyConfigService::SetProxyConfig(
    const net::ProxyConfig& proxy_config) {
  // Do nothing if the proxy configuration is unchanged.
  if (!config_pending_ && config_.Equals(proxy_config))
    return;

  config_pending_ = false;
  config_ = proxy_config;

  for (auto& observer : observers_)
    observer.OnProxyConfigChanged(config_, CONFIG_VALID);
}

void ControlledProxyConfigService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ControlledProxyConfigService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

net::ProxyConfigService::ConfigAvailability
ControlledProxyConfigService::GetLatestProxyConfig(net::ProxyConfig* config) {
  if (config_pending_) {
    *config = net::ProxyConfig();
    return CONFIG_PENDING;
  }
  *config = config_;
  return CONFIG_VALID;
}

void ControlledProxyConfigService::OnLazyPoll() {
  if (proxy_poller_)
    proxy_poller_->OnLazyPoll();
}

}  // namespace content
