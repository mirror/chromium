// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_NET_PROXY_CONFIG_MONITOR_H_
#define CHROME_BROWSER_NET_PROXY_CONFIG_MONITOR_H_

#include <memory>

#include "base/macros.h"
#include "content/public/common/network_service.mojom.h"
#include "content/public/common/proxy_config.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/proxy/proxy_config_service.h"

namespace net {
class ProxyConfig;
}

class Profile;
class PrefProxyConfigTracker;

// Tracks the ProxyConfig to use, and passes any updates to a NetworkContext's
// ProxyConfigClient.
class ProxyConfigMonitor : public net::ProxyConfigService::Observer,
                           public content::mojom::ProxyConfigPollerClient {
 public:
  // Creates a ProxyConfigMonitor that gets settings from |profile| and passes
  // them the NetworkService created using |network_context_params|. |profile|
  // may be nullptr if there's no associated Profile. The created
  // ProxyConfigMonitor must be destroyed before |profile|.
  ProxyConfigMonitor(
      Profile* profile,
      content::mojom::NetworkContextParams* network_context_params);
  ~ProxyConfigMonitor() override;

  // Flushes all pending data on the pipe, blocking the current thread until
  // they're received, to allow tests to wait until all pending proxy
  // configuration changes have been applied.
  void FlushForTesting();

 private:
  // net::ProxyConfigService::Observer implementation:
  void OnProxyConfigChanged(
      const net::ProxyConfig& config,
      net::ProxyConfigService::ConfigAvailability availability) override;

  // content::mojom::ProxyConfigPollerClient implementation:
  void OnLazyProxyConfigPoll() override;

  std::unique_ptr<net::ProxyConfigService> proxy_config_service_;
  // Monitors global and Profile prefs related to proxy configuration.
  std::unique_ptr<PrefProxyConfigTracker> pref_proxy_config_tracker_;

  mojo::Binding<content::mojom::ProxyConfigPollerClient> binding_;

  content::mojom::ProxyConfigClientPtr proxy_config_client_;

  DISALLOW_COPY_AND_ASSIGN(ProxyConfigMonitor);
};

#endif  // CHROME_BROWSER_NET_PROXY_CONFIG_MONITOR_H_
