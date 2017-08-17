// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service_impl.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/media/router/discovery/mdns/retry_strategy.h"
#include "chrome/browser/media/router/discovery/media_sink_service_base.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "chrome/common/media_router/discovery/media_sink_service.h"
#include "chrome/common/media_router/media_sink.h"
#include "components/cast_channel/cast_socket_service.h"
#include "components/net_log/chrome_net_log.h"

namespace {

media_router::MediaSinkInternal CreateCastSinkFromDialSink(
    const media_router::MediaSinkInternal& dial_sink) {
  const std::string& unique_id = dial_sink.sink().id();
  const std::string& friendly_name = dial_sink.sink().name();
  media_router::MediaSink sink(unique_id, friendly_name,
                               media_router::SinkIconType::CAST);

  media_router::CastSinkExtraData extra_data;
  extra_data.ip_address = dial_sink.dial_data().ip_address;
  extra_data.port = media_router::CastMediaSinkServiceImpl::kCastControlPort;
  extra_data.model_name = dial_sink.dial_data().model_name;
  extra_data.discovered_by_dial = true;
  extra_data.capabilities = cast_channel::CastDeviceCapability::NONE;

  return media_router::MediaSinkInternal(sink, extra_data);
}

}  // namespace

namespace media_router {

namespace {

bool IsNetworkIdUnknownOrDisconnected(const std::string& network_id) {
  return network_id == DiscoveryNetworkMonitor::kNetworkIdUnknown ||
         network_id == DiscoveryNetworkMonitor::kNetworkIdDisconnected;
}

}  // namespace

// static
const int CastMediaSinkServiceImpl::kCastControlPort = 8009;
const int kMaxAttempts = 3;
const int kDelayInSeconds = 5;

CastMediaSinkServiceImpl::CastMediaSinkServiceImpl(
    const OnSinksDiscoveredCallback& callback,
    cast_channel::CastSocketService* cast_socket_service,
    DiscoveryNetworkMonitor* network_monitor)
    : MediaSinkServiceBase(callback),
      cast_socket_service_(cast_socket_service),
      network_monitor_(network_monitor),
      retry_strategy_(
          RetryStrategy::UniformDelay(kMaxAttempts, kDelayInSeconds)) {
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

void CastMediaSinkServiceImpl::SetRetryStrategyForTest(
    std::unique_ptr<RetryStrategy> retry_strategy) {
  retry_strategy_ = std::move(retry_strategy);
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

void CastMediaSinkServiceImpl::OnFetchCompleted() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  current_sinks_.clear();

  // Copy cast sink from mDNS service to |current_sinks_|.
  for (const auto& sink_it : current_sinks_by_mdns_) {
    DVLOG(2) << "Discovered by mdns [name]: " << sink_it.second.sink().name()
             << " [ip_address]: "
             << sink_it.second.cast_data().ip_address.ToString();
    current_sinks_.insert(sink_it.second);
  }

  // Copy cast sink from DIAL discovery to |current_sinks_|.
  for (const auto& sink_it : current_sinks_by_dial_) {
    DVLOG(2) << "Discovered by dial [name]: " << sink_it.second.sink().name()
             << " [ip_address]: "
             << sink_it.second.cast_data().ip_address.ToString();
    if (!base::ContainsKey(current_sinks_by_mdns_, sink_it.first))
      current_sinks_.insert(sink_it.second);
  }

  MediaSinkServiceBase::OnFetchCompleted();
}

void CastMediaSinkServiceImpl::RecordDeviceCounts() {
  metrics_.RecordDeviceCountsIfNeeded(current_sinks_.size(),
                                      current_service_ip_endpoints_.size());
}

void CastMediaSinkServiceImpl::OpenChannels(
    std::vector<MediaSinkInternal> cast_sinks) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  current_service_ip_endpoints_.clear();

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

  if (!retry_strategy_) {
    OnChannelOpenFailed(socket, error_state);
    return;
  }

  // Only reopen cast channel if cast channel is already opened.
  const net::IPEndPoint& ip_endpoint = socket.ip_endpoint();
  DVLOG(2) << "Start reopen cast channel: " << ip_endpoint.ToString();

  // find existing cast sink from |current_sinks_by_mdns_|.
  auto cast_sink_it = current_sinks_by_mdns_.find(ip_endpoint.address());
  if (cast_sink_it != current_sinks_by_mdns_.end()) {
    OpenChannel(ip_endpoint, cast_sink_it->second);
    return;
  }

  // find existing cast sink from |current_sinks_by_dial_|.
  auto cast_sink_by_dial_it =
      current_sinks_by_dial_.find(ip_endpoint.address());
  if (cast_sink_by_dial_it != current_sinks_by_dial_.end()) {
    OpenChannel(ip_endpoint, cast_sink_by_dial_it->second);
    return;
  }

  DVLOG(2) << "Cannot find existing cast sink. Skip reopen cast channel: "
           << ip_endpoint.ToString();
}

void CastMediaSinkServiceImpl::OnMessage(
    const cast_channel::CastSocket& socket,
    const cast_channel::CastMessage& message) {}

void CastMediaSinkServiceImpl::OnNetworksChanged(
    const std::string& network_id) {
  std::string last_network_id = current_network_id_;
  current_network_id_ = network_id;
  if (IsNetworkIdUnknownOrDisconnected(network_id)) {
    if (!IsNetworkIdUnknownOrDisconnected(last_network_id)) {
      // Collect current sinks even if OnFetchCompleted hasn't collected the
      // latest sinks.
      std::vector<MediaSinkInternal> current_sinks;
      for (const auto& sink_it : current_sinks_by_mdns_) {
        current_sinks.push_back(sink_it.second);
      }
      for (const auto& sink_it : current_sinks_by_dial_) {
        if (!base::ContainsKey(current_sinks_by_mdns_, sink_it.first))
          current_sinks.push_back(sink_it.second);
      }
      sink_cache_[last_network_id] = std::move(current_sinks);
    }
    return;
  }
  auto cache_entry = sink_cache_.find(network_id);
  // Check if we have any cached sinks for this network ID.
  if (cache_entry == sink_cache_.end())
    return;

  DVLOG(2) << "Cache restored " << cache_entry->second.size()
           << " sink(s) for network " << network_id;
  OpenChannels(cache_entry->second);
}

void CastMediaSinkServiceImpl::OpenChannel(const net::IPEndPoint& ip_endpoint,
                                           MediaSinkInternal cast_sink) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto pending_sink_it = pending_cast_sinks_.find(ip_endpoint);
  if (pending_sink_it != pending_cast_sinks_.end()) {
    if (pending_sink_it->second == cast_sink) {
      DVLOG(2) << "Opening channel pending for: " << ip_endpoint.ToString()
               << ". Skip opening...";
      return;
    }
    // |cast_sink| is different from what we have cached, e.g. due to IP
    // address changes.
    pending_cast_sinks_.erase(pending_sink_it);
  }

  DVLOG(2) << "Start OpenChannel " << ip_endpoint.ToString()
           << " name: " << cast_sink.sink().name();
  pending_cast_sinks_.insert(std::make_pair(ip_endpoint, cast_sink));
  OpenChannelWithRetry(ip_endpoint, 0);
}

void CastMediaSinkServiceImpl::OpenChannelWithRetry(
    const net::IPEndPoint& ip_endpoint,
    int num_attempts) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(2) << "OpenChannelWithRetry [attempt]: " << num_attempts
           << " [ip_endpoint]: " << ip_endpoint.ToString()
           << " [time]: " << base::Time::Now();

  cast_socket_service_->OpenSocket(
      ip_endpoint, g_browser_process->net_log(),
      base::BindOnce(&CastMediaSinkServiceImpl::OnChannelOpened, AsWeakPtr(),
                     num_attempts),
      this);
}

void CastMediaSinkServiceImpl::OnChannelOpened(
    int num_attempts,
    cast_channel::CastSocket* socket) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(socket);
  const net::IPEndPoint& ip_endpoint = socket->ip_endpoint();

  if (socket->error_state() == cast_channel::ChannelError::NONE) {
    DVLOG(2) << "Open channel " << ip_endpoint.ToString();
    OnChannelOpenSucceeded(socket);
    return;
  }

  if (!retry_strategy_ || !retry_strategy_->TryAgain(num_attempts)) {
    DVLOG(2) << "Fail to open channel " << ip_endpoint.ToString();
    OnChannelOpenFailed(*socket, socket->error_state());
    return;
  }

  int delay_in_seconds = retry_strategy_->GetDelayInSeconds(num_attempts);
  DVLOG(2) << "Try to reopen: " << ip_endpoint.ToString() << " in ["
           << delay_in_seconds << "] seconds"
           << " [current time]: " << base::Time::Now();

  content::BrowserThread::PostDelayedTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&CastMediaSinkServiceImpl::OpenChannelWithRetry,
                     AsWeakPtr(), ip_endpoint, ++num_attempts),
      base::TimeDelta::FromSeconds(delay_in_seconds));
}

void CastMediaSinkServiceImpl::OnChannelOpenSucceeded(
    cast_channel::CastSocket* socket) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(socket);

  const net::IPEndPoint& ip_endpoint = socket->ip_endpoint();
  auto sink_it = pending_cast_sinks_.find(ip_endpoint);
  if (sink_it == pending_cast_sinks_.end()) {
    DVLOG(1) << "No pending cast sink for: " << ip_endpoint.ToString();
    return;
  }

  MediaSinkInternal cast_sink = sink_it->second;
  pending_cast_sinks_.erase(sink_it);

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

  auto& ip_address = cast_sink.cast_data().ip_address;
  // Add or update existing cast sink.
  if (updated_sink.cast_data().discovered_by_dial) {
    current_sinks_by_dial_[ip_address] = updated_sink;
  } else {
    current_sinks_by_mdns_[ip_address] = updated_sink;
  }
  MediaSinkServiceBase::RestartTimer();
}

void CastMediaSinkServiceImpl::OnChannelOpenFailed(
    const cast_channel::CastSocket& socket,
    cast_channel::ChannelError error_state) {
  const net::IPEndPoint& ip_endpoint = socket.ip_endpoint();
  DVLOG(1) << "Fail to open channel: " << ip_endpoint.ToString()
           << " [error_state]: "
           << cast_channel::ChannelErrorToString(error_state);
  pending_cast_sinks_.erase(ip_endpoint);

  auto& ip_address = ip_endpoint.address();
  current_sinks_by_dial_.erase(ip_address);
  current_sinks_by_mdns_.erase(ip_address);
  MediaSinkServiceBase::RestartTimer();
}

void CastMediaSinkServiceImpl::OnDialSinkAdded(const MediaSinkInternal& sink) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  net::IPEndPoint ip_endpoint(sink.dial_data().ip_address, kCastControlPort);

  if (base::ContainsKey(current_service_ip_endpoints_, ip_endpoint)) {
    DVLOG(2) << "Sink discovered by mDNS, skip adding [name]: "
             << sink.sink().name();
    return;
  }

  // TODO(crbug.com/753175): Dual discovery should not try to open cast channel
  // for non-Cast device.
  OpenChannel(ip_endpoint, CreateCastSinkFromDialSink(sink));
}

}  // namespace media_router
