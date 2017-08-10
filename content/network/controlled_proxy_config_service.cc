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
    const base::Optional<net::ProxyConfig>& proxy_config) {
  if (!proxy_config) {
    if (availability_ == CONFIG_PENDING)
      return;
    availability_ = CONFIG_PENDING;
    config_ = net::ProxyConfig();
  } else {
    if (availability_ == CONFIG_VALID && config_.Equals(*proxy_config)) {
      return;
    }
    availability_ = CONFIG_VALID;
    config_ = *proxy_config;
  }

  for (auto& observer : observers_)
    observer.OnProxyConfigChanged(config_, availability_);
}

void ControlledProxyConfigService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void ControlledProxyConfigService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

net::ProxyConfigService::ConfigAvailability
ControlledProxyConfigService::GetLatestProxyConfig(net::ProxyConfig* config) {
  if (availability_ == CONFIG_VALID) {
    *config = config_;
  } else {
    *config = net::ProxyConfig();
  }
  return availability_;
}

void ControlledProxyConfigService::OnLazyPoll() {
  if (proxy_poller_)
    proxy_poller_->OnLazyPoll();
}

}  // namespace content
