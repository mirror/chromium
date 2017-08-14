// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_context.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/optional.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "components/network_session_configurator/browser/network_session_configurator.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "content/network/cache_url_loader.h"
#include "content/network/controlled_proxy_config_service.h"
#include "content/network/network_service_impl.h"
#include "content/network/network_service_url_loader_factory_impl.h"
#include "content/network/url_loader_impl.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_switches.h"
#include "net/dns/host_resolver.h"
#include "net/dns/mapped_host_resolver.h"
#include "net/http/http_network_session.h"
#include "net/http/http_transaction_factory.h"
#include "net/proxy/proxy_config.h"
#include "net/proxy/proxy_config_service_fixed.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"

namespace content {

NetworkContext::NetworkContext(NetworkServiceImpl* network_service,
                               mojom::NetworkContextRequest request,
                               mojom::NetworkContextParamsPtr params)
    : network_service_(network_service),
      params_(std::move(params)),
      binding_(this, std::move(request)) {
  owned_url_request_context_ = MakeURLRequestContext(params_.get());
  url_request_context_ = owned_url_request_context_.get();
  network_service_->RegisterNetworkContext(this);
  binding_.set_connection_error_handler(
      base::Bind(&NetworkContext::OnConnectionError, base::Unretained(this)));
}

// TODO(mmenke): Share URLRequestContextBulder configuration between two
// constructors. Can only share them once consumer code is ready for its
// corresponding options to be overwritten.
NetworkContext::NetworkContext(
    NetworkServiceImpl* network_service,
    mojom::NetworkContextRequest request,
    mojom::NetworkContextParamsPtr params,
    std::unique_ptr<net::URLRequestContextBuilder> builder)
    : network_service_(network_service),
      params_(std::move(params)),
      binding_(this, std::move(request)) {
  network_service_->RegisterNetworkContext(this);
  ApplyContextParamsToBuilder(builder.get(), params_.get());
  owned_url_request_context_ = builder->Build();
  url_request_context_ = owned_url_request_context_.get();
}

NetworkContext::NetworkContext(mojom::NetworkContextRequest request,
                               net::URLRequestContext* url_request_context)
    : network_service_(nullptr), binding_(this, std::move(request)) {
  url_request_context_ = url_request_context;
}

NetworkContext::~NetworkContext() {
  // Call each URLLoaderImpl and ask it to release its net::URLRequest, as the
  // corresponding net::URLRequestContext is going away with this
  // NetworkContext. The loaders can be deregistering themselves in Cleanup(),
  // so have to be careful.
  while (!url_loaders_.empty())
    (*url_loaders_.begin())->Cleanup();

  // May be nullptr in tests.
  if (network_service_)
    network_service_->DeregisterNetworkContext(this);
}

std::unique_ptr<NetworkContext> NetworkContext::CreateForTesting() {
  mojom::NetworkContextParamsPtr params = mojom::NetworkContextParams::New();
  // Disable proxy configuration detection, to avoid dependencies on local
  // network configuration.
  params->auto_detect_proxy_config = false;
  params->proxy_config = net::ProxyConfig::CreateDirect();

  return base::WrapUnique(new NetworkContext(std::move(params)));
}

void NetworkContext::RegisterURLLoader(URLLoaderImpl* url_loader) {
  DCHECK(url_loaders_.count(url_loader) == 0);
  url_loaders_.insert(url_loader);
}

void NetworkContext::DeregisterURLLoader(URLLoaderImpl* url_loader) {
  size_t removed_count = url_loaders_.erase(url_loader);
  DCHECK(removed_count);
}

void NetworkContext::CreateURLLoaderFactory(
    mojom::URLLoaderFactoryRequest request,
    uint32_t process_id) {
  loader_factory_bindings_.AddBinding(
      base::MakeUnique<NetworkServiceURLLoaderFactoryImpl>(this, process_id),
      std::move(request));
}

void NetworkContext::HandleViewCacheRequest(const GURL& url,
                                            mojom::URLLoaderClientPtr client) {
  StartCacheURLLoader(url, url_request_context_, std::move(client));
}

void NetworkContext::SetProxyConfig(const net::ProxyConfig& proxy_config) {
  DCHECK(controlled_proxy_config_service_) << "Shouldn't be called when "
                                              "automatic proxy config "
                                              "detection is enabled.";
  controlled_proxy_config_service_->SetProxyConfig(proxy_config);
}

void NetworkContext::DisableQuic() {
  url_request_context_->http_transaction_factory()->GetSession()->DisableQuic();
}

void NetworkContext::Cleanup() {
  // The NetworkService is going away, so have to destroy the
  // net::URLRequestContext held by this NetworkContext.
  delete this;
}

NetworkContext::NetworkContext(mojom::NetworkContextParamsPtr params)
    : network_service_(nullptr), params_(std::move(params)), binding_(this) {
  owned_url_request_context_ = MakeURLRequestContext(params_.get());
  url_request_context_ = owned_url_request_context_.get();
}

void NetworkContext::OnConnectionError() {
  // Don't delete |this| in response to connection errors when it was created by
  // CreateForTesting.
  if (network_service_)
    delete this;
}

void NetworkContext::ApplyContextParamsToBuilder(
    net::URLRequestContextBuilder* builder,
    mojom::NetworkContextParams* network_context_params) {
  if (!network_context_params->auto_detect_proxy_config) {
    std::unique_ptr<ControlledProxyConfigService> proxy_config_service =
        base::MakeUnique<ControlledProxyConfigService>(
            std::move(network_context_params->proxy_config_poller));
    controlled_proxy_config_service_ = proxy_config_service.get();
    if (network_context_params->proxy_config) {
      controlled_proxy_config_service_->SetProxyConfig(
          *network_context_params->proxy_config);
    }
    builder->set_proxy_config_service(std::move(proxy_config_service));
  }

  if (!network_context_params->http_cache_enabled) {
    builder->DisableHttpCache();
  } else {
    net::URLRequestContextBuilder::HttpCacheParams cache_params;
    cache_params.max_size = network_context_params->http_cache_max_size;
    if (!network_context_params->http_cache_path) {
      cache_params.type =
          net::URLRequestContextBuilder::HttpCacheParams::IN_MEMORY;
    } else {
      cache_params.path = *network_context_params->http_cache_path;
      cache_params.type = network_session_configurator::ChooseCacheType(
          *base::CommandLine::ForCurrentProcess());
    }

    builder->EnableHttpCache(cache_params);
  }

  builder->set_data_enabled(network_context_params->enable_data_url_support);
#if !BUILDFLAG(DISABLE_FILE_SUPPORT)
  builder->set_file_enabled(network_context_params->enable_file_url_support);
#else  // BUILDFLAG(DISABLE_FILE_SUPPORT)
  DCHECK(!network_context_params->enable_file_url_support);
#endif
#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
  builder->set_ftp_enabled(network_context_params->enable_ftp_url_support);
#else  // BUILDFLAG(DISABLE_FTP_SUPPORT)
  DCHECK(!network_context_params->enable_ftp_url_support);
#endif

  net::HttpNetworkSession::Params session_params;
  bool is_quic_force_disabled = false;
  if (network_service_ && network_service_->quic_disabled())
    is_quic_force_disabled = true;

  network_session_configurator::ParseCommandLineAndFieldTrials(
      *base::CommandLine::ForCurrentProcess(), is_quic_force_disabled,
      network_context_params->quic_user_agent_id, &session_params);

  session_params.http_09_on_non_default_ports_enabled =
      network_context_params->http_09_on_non_default_ports_enabled;

  builder->set_http_network_session_params(session_params);
}

std::unique_ptr<net::URLRequestContext> NetworkContext::MakeURLRequestContext(
    mojom::NetworkContextParams* network_context_params) {
  net::URLRequestContextBuilder builder;
  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();

  if (command_line->HasSwitch(switches::kHostResolverRules)) {
    std::unique_ptr<net::HostResolver> host_resolver(
        net::HostResolver::CreateDefaultResolver(nullptr));
    std::unique_ptr<net::MappedHostResolver> remapped_host_resolver(
        new net::MappedHostResolver(std::move(host_resolver)));
    remapped_host_resolver->SetRulesFromString(
        command_line->GetSwitchValueASCII(switches::kHostResolverRules));
    builder.set_host_resolver(std::move(remapped_host_resolver));
  }
  builder.set_accept_language("en-us,en");
  builder.set_user_agent(GetContentClient()->GetUserAgent());

  net::ProxyConfig config;
  if (command_line->HasSwitch(switches::kProxyServer)) {
    config.proxy_rules().ParseFromString(
        command_line->GetSwitchValueASCII(switches::kProxyServer));
  } else {
    config = net::ProxyConfig::CreateDirect();
  }
  std::unique_ptr<net::ProxyConfigService> fixed_config_service =
      base::MakeUnique<net::ProxyConfigServiceFixed>(config);
  builder.set_proxy_config_service(std::move(fixed_config_service));

  ApplyContextParamsToBuilder(&builder, network_context_params);

  return builder.Build();
}

}  // namespace content
