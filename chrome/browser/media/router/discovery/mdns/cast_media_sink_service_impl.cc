// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service_impl.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "components/cast_channel/cast_socket_service.h"
#include "components/net_log/chrome_net_log.h"

namespace media_router {

// static
const int CastMediaSinkServiceImpl::kCastControlPort = 8009;

CastMediaSinkServiceImpl::CastMediaSinkServiceImpl(
    const OnSinksDiscoveredCallback& callback,
    cast_channel::CastSocketService* cast_socket_service,
    DiscoveryNetworkMonitor* network_monitor)
    : MediaSinkServiceBase(callback),
      cast_socket_service_(cast_socket_service),
      network_monitor_(network_monitor) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  DCHECK(cast_socket_service_);
  DCHECK(network_monitor_);
  network_monitor_->AddObserver(this);
}

CastMediaSinkServiceImpl::~CastMediaSinkServiceImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  cast_channel::CastSocketService::GetInstance()->RemoveObserver(this);
  network_monitor_->RemoveObserver(this);
}

// MediaSinkService implementation
void CastMediaSinkServiceImpl::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  MediaSinkServiceBase::StartTimer();
}

void CastMediaSinkServiceImpl::Stop() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  MediaSinkServiceBase::StopTimer();
}

void CastMediaSinkServiceImpl::RecordDeviceCounts() {
  metrics_.RecordDeviceCountsIfNeeded(current_sinks_.size(),
                                      current_service_ip_endpoints_.size());
}

void CastMediaSinkServiceImpl::OpenChannels(
    std::vector<MediaSinkInternal> cast_sinks) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  for (const auto& cast_sink : cast_sinks) {
    net::IPEndPoint ip_endpoint(cast_sink.cast_data().ip_address,
                                cast_sink.cast_data().port);
    current_service_ip_endpoints_.insert(ip_endpoint);
    OpenChannel(ip_endpoint, cast_sink);
  }

  MediaSinkServiceBase::RestartTimer();
}

void CastMediaSinkServiceImpl::OnError(const cast_channel::CastSocket& socket,
                                       cast_channel::ChannelError error_state) {
  DVLOG(1) << "OnError [ip_endpoint]: " << socket.ip_endpoint().ToString()
           << " [error_state]: "
           << cast_channel::ChannelErrorToString(error_state);
  net::IPEndPoint ip_endpoint = socket.ip_endpoint();
  auto sink_it = std::find_if(
      current_sinks_.begin(), current_sinks_.end(),
      [&](const MediaSinkInternal& sink) {
        return sink.cast_data().ip_address == ip_endpoint.address() &&
               sink.cast_data().port == ip_endpoint.port();
      });

  if (sink_it != current_sinks_.end())
    current_sinks_.erase(sink_it);

  auto endpoint_it = current_service_ip_endpoints_.find(ip_endpoint);
  if (endpoint_it != current_service_ip_endpoints_.end())
    current_service_ip_endpoints_.erase(endpoint_it);

  if (sink_it != current_sinks_.end())
    MediaSinkServiceBase::RestartTimer();
}

void CastMediaSinkServiceImpl::OnMessage(
    const cast_channel::CastSocket& socket,
    const cast_channel::CastMessage& message) {}

void CastMediaSinkServiceImpl::OpenChannel(const net::IPEndPoint& ip_endpoint,
                                           MediaSinkInternal cast_sink) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(2) << "OpenChannel " << ip_endpoint.ToString()
           << " name: " << cast_sink.sink().name();

  cast_socket_service_->OpenSocket(
      ip_endpoint, g_browser_process->net_log(),
      base::BindOnce(&CastMediaSinkServiceImpl::OnChannelOpened, AsWeakPtr(),
                     std::move(cast_sink)),
      this);
}

void CastMediaSinkServiceImpl::OnNetworksChanged(
    const std::string& network_id) {
  std::string last_network_id = current_network_id_;
  current_network_id_ = network_id;
  if (network_id == DiscoveryNetworkMonitor::kNetworkIdUnknown ||
      network_id == DiscoveryNetworkMonitor::kNetworkIdDisconnected) {
    if (last_network_id == DiscoveryNetworkMonitor::kNetworkIdUnknown ||
        last_network_id == DiscoveryNetworkMonitor::kNetworkIdDisconnected) {
      return;
    }
    auto cache_entry = sink_cache_.find(last_network_id);
    if (cache_entry == sink_cache_.end()) {
      sink_cache_.emplace(last_network_id,
                          std::vector<MediaSinkInternal>(current_sinks_.begin(),
                                                         current_sinks_.end()));
    } else {
      cache_entry->second = std::vector<MediaSinkInternal>(
          current_sinks_.begin(), current_sinks_.end());
    }
    return;
  }
  auto cache_entry = sink_cache_.find(network_id);
  // Check if we have any cached sinks for this network ID.
  if (cache_entry == sink_cache_.end()) {
    return;
  }
  DVLOG(2) << "Cache restored " << cache_entry->second.size()
           << " sink(s) for network " << network_id;
  OpenChannels(cache_entry->second);
}

void CastMediaSinkServiceImpl::OnChannelOpened(
    MediaSinkInternal cast_sink,
    cast_channel::CastSocket* socket) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(socket);
  if (socket->error_state() != cast_channel::ChannelError::NONE) {
    DVLOG(2) << "Fail to open channel "
             << cast_sink.cast_data().ip_address.ToString()
             << " [name]: " << cast_sink.sink().name();
    return;
  }

  media_router::CastSinkExtraData extra_data = cast_sink.cast_data();
  extra_data.capabilities = cast_channel::CastDeviceCapability::AUDIO_OUT;
  if (!socket->audio_only())
    extra_data.capabilities |= cast_channel::CastDeviceCapability::VIDEO_OUT;
  extra_data.cast_channel_id = socket->id();
  MediaSinkInternal updated_sink(cast_sink.sink(), extra_data);
  DVLOG(2) << "Ading sink to current_sinks_ [name]: "
           << updated_sink.sink().name();

  current_sinks_.insert(updated_sink);
  MediaSinkServiceBase::RestartTimer();
}

}  // namespace media_router
