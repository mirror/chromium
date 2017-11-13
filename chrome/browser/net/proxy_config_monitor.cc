// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/proxy_config_monitor.h"

#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/proxy_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "content/public/browser/browser_thread.h"
#include "mojo/public/cpp/bindings/associated_interface_ptr.h"

#if defined(OS_CHROMEOS)
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#endif  // defined(OS_CHROMEOS)

ProxyConfigMonitor::ProxyConfigMonitor(
    Profile* profile,
    content::mojom::NetworkContextParams* network_context_params)
    : binding_(this) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  // If there's no Profile, or this is the ChromeOS signing profile, just create
  // the tracker from global state.
  if (!profile
#if defined(OS_CHROMEOS)
      || chromeos::ProfileHelper::IsSigninProfile(profile)
#endif  // defined(OS_CHROMEOS)
          ) {
    pref_proxy_config_tracker_.reset(
        ProxyServiceFactory::CreatePrefProxyConfigTrackerOfLocalState(
            g_browser_process->local_state()));
  } else {
    pref_proxy_config_tracker_.reset(
        ProxyServiceFactory::CreatePrefProxyConfigTrackerOfProfile(
            profile->GetPrefs(), g_browser_process->local_state()));
  }
  proxy_config_service_ = ProxyServiceFactory::CreateProxyConfigService(
      pref_proxy_config_tracker_.get());

  network_context_params->proxy_config_client_request =
      mojo::MakeRequest(&proxy_config_client_);
  binding_.Bind(
      mojo::MakeRequest(&network_context_params->proxy_config_poller_client));

  proxy_config_service_->AddObserver(this);
  net::ProxyConfig proxy_config;
  net::ProxyConfigService::ConfigAvailability availability =
      proxy_config_service_->GetLatestProxyConfig(&proxy_config);
  if (availability != net::ProxyConfigService::CONFIG_PENDING)
    OnProxyConfigChanged(proxy_config, availability);
}

ProxyConfigMonitor::~ProxyConfigMonitor() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  proxy_config_service_->RemoveObserver(this);
  pref_proxy_config_tracker_->DetachFromPrefService();
}

void ProxyConfigMonitor::FlushForTesting() {
  proxy_config_client_.FlushForTesting();
}

void ProxyConfigMonitor::OnProxyConfigChanged(
    const net::ProxyConfig& config,
    net::ProxyConfigService::ConfigAvailability availability) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  switch (availability) {
    case net::ProxyConfigService::CONFIG_VALID:
      proxy_config_ = config;
      proxy_config_client_->OnProxyConfigUpdated(config);
      break;
    case net::ProxyConfigService::CONFIG_UNSET:
      proxy_config_client_->OnProxyConfigUpdated(
          net::ProxyConfig::CreateDirect());
      break;
    case net::ProxyConfigService::CONFIG_PENDING:
      NOTREACHED();
      break;
  }
}

void ProxyConfigMonitor::OnLazyProxyConfigPoll() {
  proxy_config_service_->OnLazyPoll();
}
