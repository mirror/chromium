// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "components/cast_channel/cast_socket_service.h"
#include "components/net_log/chrome_net_log.h"
#include "content/public/common/content_client.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_address.h"

namespace {

enum ErrorType {
  NONE,
  NOT_CAST_DEVICE,
  MISSING_ID,
  MISSING_FRIENDLY_NAME,
  MISSING_OR_INVALID_IP_ADDRESS,
  MISSING_OR_INVALID_PORT,
};

ErrorType CreateCastMediaSink(const media_router::DnsSdService& service,
                              int channel_id,
                              bool audio_only,
                              media_router::MediaSinkInternal* cast_sink) {
  DCHECK(cast_sink);
  if (service.service_name.find(
          media_router::CastMediaSinkService::kCastServiceType) ==
      std::string::npos)
    return ErrorType::NOT_CAST_DEVICE;

  net::IPAddress ip_address;
  if (!ip_address.AssignFromIPLiteral(service.ip_address))
    return ErrorType::MISSING_OR_INVALID_IP_ADDRESS;

  std::map<std::string, std::string> service_data;
  for (const auto& item : service.service_data) {
    // |item| format should be "id=xxxxxx", etc.
    size_t split_idx = item.find('=');
    if (split_idx == std::string::npos)
      continue;

    std::string key = item.substr(0, split_idx);
    std::string val =
        split_idx < item.length() ? item.substr(split_idx + 1) : "";
    service_data[key] = val;
  }

  // When use this "sink" within browser, please note it will have a different
  // ID when it is sent to the extension, because it derives a different sink ID
  // using the given sink ID.
  std::string unique_id = service_data["id"];
  if (unique_id.empty())
    return ErrorType::MISSING_ID;
  std::string friendly_name = service_data["fn"];
  if (friendly_name.empty())
    return ErrorType::MISSING_FRIENDLY_NAME;
  media_router::MediaSink sink(unique_id, friendly_name,
                               media_router::SinkIconType::CAST);

  media_router::CastSinkExtraData extra_data;
  extra_data.ip_address = ip_address;
  extra_data.model_name = service_data["md"];
  extra_data.capabilities = cast_channel::CastDeviceCapability::AUDIO_OUT;
  if (!audio_only)
    extra_data.capabilities |= cast_channel::CastDeviceCapability::VIDEO_OUT;
  extra_data.cast_channel_id = channel_id;

  cast_sink->set_sink(sink);
  cast_sink->set_cast_data(extra_data);

  return ErrorType::NONE;
}

// Predicate to test if two discovered services have the same service_name.
class IsSameServiceName {
 public:
  explicit IsSameServiceName(const media_router::DnsSdService& service)
      : service_(service) {}
  bool operator()(const media_router::DnsSdService& other) const {
    return service_.service_name == other.service_name;
  }

 private:
  const media_router::DnsSdService& service_;
};

}  // namespace

namespace media_router {

// static
const char CastMediaSinkService::kCastServiceType[] = "_googlecast._tcp.local";

CastMediaSinkService::CastMediaSinkService(
    const OnSinksDiscoveredCallback& callback,
    content::BrowserContext* browser_context)
    : MediaSinkServiceBase(callback),
      cast_socket_service_(cast_channel::CastSocketService::GetInstance()) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(cast_socket_service_);
}

CastMediaSinkService::CastMediaSinkService(
    const OnSinksDiscoveredCallback& callback,
    cast_channel::CastSocketService* cast_socket_service,
    DiscoveryNetworkMonitor* network_monitor)
    : MediaSinkServiceBase(callback),
      cast_socket_service_(cast_socket_service),
      network_monitor_(network_monitor) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(cast_socket_service_);
  DCHECK(network_monitor_);
  network_monitor_->AddObserver(this);
}

CastMediaSinkService::~CastMediaSinkService() {}

void CastMediaSinkService::Start() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (dns_sd_registry_)
    return;

  dns_sd_registry_ = DnsSdRegistry::GetInstance();
  dns_sd_registry_->AddObserver(this);
  dns_sd_registry_->RegisterDnsSdListener(kCastServiceType);
  network_monitor_ = DiscoveryNetworkMonitor::GetInstance();
  network_monitor_->AddObserver(this);
  MediaSinkServiceBase::StartTimer();
}

void CastMediaSinkService::Stop() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (!dns_sd_registry_)
    return;

  dns_sd_registry_->UnregisterDnsSdListener(kCastServiceType);
  dns_sd_registry_->RemoveObserver(this);
  dns_sd_registry_ = nullptr;
  network_monitor_->RemoveObserver(this);
  network_monitor_ = nullptr;
  MediaSinkServiceBase::StopTimer();
}

void CastMediaSinkService::SetDnsSdRegistryForTest(DnsSdRegistry* registry) {
  DCHECK(!dns_sd_registry_);
  dns_sd_registry_ = registry;
  dns_sd_registry_->AddObserver(this);
  dns_sd_registry_->RegisterDnsSdListener(kCastServiceType);
  MediaSinkServiceBase::StartTimer();
}

void CastMediaSinkService::StartDeviceResolution(
    const DnsSdRegistry::DnsSdServiceList& services_to_resolve) {
  for (const auto& service : services_to_resolve) {
    net::IPAddress ip_address;
    if (!ip_address.AssignFromIPLiteral(service.ip_address)) {
      DVLOG(2) << "Invalid ip_addresss: " << service.ip_address;
      services_pending_resolution_.erase(
          std::remove(services_pending_resolution_.begin(),
                      services_pending_resolution_.end(), service));
      continue;
    }

    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::Bind(
            &CastMediaSinkService::OpenChannelOnIOThread, this, service,
            net::IPEndPoint(ip_address, service.service_host_port.port())));
  }

  MediaSinkServiceBase::RestartTimer();
}

void CastMediaSinkService::OnDnsSdEvent(
    const std::string& service_type,
    const DnsSdRegistry::DnsSdServiceList& services) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DVLOG(2) << "CastMediaSinkService::OnDnsSdEvent found " << services.size()
           << " services";

  for (const auto& service : services) {
    auto service_it = std::find_if(services_pending_resolution_.begin(),
                                   services_pending_resolution_.end(),
                                   IsSameServiceName(service));
    if (service_it == services_pending_resolution_.end()) {
      services_pending_resolution_.push_back(service);
    } else {
      *service_it = service;
    }
  }
  StartDeviceResolution(services);
}

void CastMediaSinkService::OpenChannelOnIOThread(
    const DnsSdService& service,
    const net::IPEndPoint& ip_endpoint) {
  if (!observer_)
    observer_.reset(new CastSocketObserver(this));

  cast_socket_service_->OpenSocket(
      ip_endpoint, g_browser_process->net_log(),
      base::Bind(&CastMediaSinkService::OnChannelOpenedOnIOThread, this,
                 service),
      observer_.get());
}

void CastMediaSinkService::OnChannelOpenedOnIOThread(
    const DnsSdService& service,
    cast_channel::CastSocket* socket) {
  DCHECK(socket);
  if (!base::ContainsValue(services_pending_resolution_, service)) {
    DVLOG(2) << "Service data not found in pending service data list...";
    return;
  }

  services_pending_resolution_.erase(
      std::remove(services_pending_resolution_.begin(),
                  services_pending_resolution_.end(), service));
  if (socket->error_state() != cast_channel::ChannelError::NONE) {
    DVLOG(2) << "Fail to open channel " << service.ip_address << ": "
             << service.service_host_port.ToString() << " [ChannelError]: "
             << cast_channel::ChannelErrorToString(socket->error_state());
    return;
  }

  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(&CastMediaSinkService::OnChannelOpenedOnUIThread, this,
                 service, socket->id(), socket->audio_only()));
}

void CastMediaSinkService::OnChannelOpenedOnUIThread(
    const DnsSdService& service,
    int channel_id,
    bool audio_only) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  MediaSinkInternal sink;
  ErrorType error = CreateCastMediaSink(service, channel_id, audio_only, &sink);
  if (error != ErrorType::NONE) {
    DVLOG(2) << "Fail to create Cast device [error]: " << error;
    return;
  }

  DVLOG(2) << "Adding sink to current_sinks_ [id]: " << sink.sink().id();
  current_sinks_.insert(sink);
  current_services_.push_back(service);
  MediaSinkServiceBase::RestartTimer();
}

void CastMediaSinkService::OnNetworksChanged(const std::string& network_id) {
  std::string last_network_id = current_network_id_;
  current_network_id_ = network_id;
  if (network_id == DiscoveryNetworkMonitor::kNetworkIdUnknown ||
      network_id == DiscoveryNetworkMonitor::kNetworkIdDisconnected) {
    if (last_network_id == DiscoveryNetworkMonitor::kNetworkIdUnknown ||
        last_network_id == DiscoveryNetworkMonitor::kNetworkIdDisconnected) {
      return;
    }
    service_cache_.emplace(last_network_id, current_services_);
    return;
  }
  auto cache_entry = service_cache_.find(network_id);
  // Check if we have any cached services for this network ID.
  if (cache_entry == service_cache_.end()) {
    return;
  }
  DVLOG(2) << "Cache restored " << cache_entry->second.size()
           << " service(s) for network " << network_id;
  DnsSdRegistry::DnsSdServiceList services_to_resolve;
  for (const auto& service : cache_entry->second) {
    if (std::find_if(services_pending_resolution_.begin(),
                     services_pending_resolution_.end(),
                     IsSameServiceName(service)) ==
        services_pending_resolution_.end()) {
      services_pending_resolution_.push_back(service);
      services_to_resolve.push_back(service);
    }
  }
  current_services_ = cache_entry->second;
  StartDeviceResolution(services_to_resolve);
}

void CastMediaSinkService::OnCastChannelError(
    const net::IPEndPoint& ip_endpoint) {
  const net::IPAddress& ip_address = ip_endpoint.address();
  const std::string ip_address_str = ip_address.ToString();
  const auto port = ip_endpoint.port();
  decltype(current_sinks_)::iterator sink_it;
  while ((sink_it = std::find_if(current_sinks_.begin(), current_sinks_.end(),
                                 [&ip_address](const MediaSinkInternal& sink) {
                                   return sink.cast_data().ip_address ==
                                          ip_address;
                                 })) != current_sinks_.end()) {
    current_sinks_.erase(sink_it);
  }
  current_services_.erase(
      std::remove_if(current_services_.begin(), current_services_.end(),
                     [&ip_address_str, port](const DnsSdService& service) {
                       return service.ip_address == ip_address_str &&
                              service.service_host_port.port() == port;
                     }));
  auto cache_entry = service_cache_.find(current_network_id_);
  if (cache_entry == service_cache_.end()) {
    return;
  }
  cache_entry->second.erase(
      std::remove_if(cache_entry->second.begin(), cache_entry->second.end(),
                     [&ip_address_str, port](const DnsSdService& service) {
                       return service.ip_address == ip_address_str &&
                              service.service_host_port.port() == port;
                     }));
}

CastMediaSinkService::CastSocketObserver::CastSocketObserver(
    CastMediaSinkService* media_sink_service)
    : media_sink_service_(media_sink_service) {}

CastMediaSinkService::CastSocketObserver::~CastSocketObserver() {
  cast_channel::CastSocketService::GetInstance()->RemoveObserver(this);
}

void CastMediaSinkService::CastSocketObserver::OnError(
    const cast_channel::CastSocket& socket,
    cast_channel::ChannelError error_state) {
  DVLOG(1) << "OnError [ip_endpoint]: " << socket.ip_endpoint().ToString()
           << " [error_state]: "
           << cast_channel::ChannelErrorToString(error_state);
  media_sink_service_->OnCastChannelError(socket.ip_endpoint());
}

void CastMediaSinkService::CastSocketObserver::OnMessage(
    const cast_channel::CastSocket& socket,
    const cast_channel::CastMessage& message) {}

}  // namespace media_router
