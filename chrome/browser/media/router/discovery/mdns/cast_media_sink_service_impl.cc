// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service_impl.h"

#include "base/memory/ptr_util.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/default_clock.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/media/router/discovery/media_sink_service_base.h"
#include "chrome/browser/media/router/media_router_feature.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "chrome/common/media_router/discovery/media_sink_service.h"
#include "chrome/common/media_router/media_sink.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "components/cast_channel/cast_channel_enum.h"
#include "components/cast_channel/cast_socket_service.h"
#include "components/cast_channel/logger.h"
#include "components/net_log/chrome_net_log.h"
#include "net/base/backoff_entry.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"

namespace media_router {

using AppAvailability = CastSinkExtraData::AppAvailability;

namespace {

bool IsNetworkIdUnknownOrDisconnected(const std::string& network_id) {
  return network_id == DiscoveryNetworkMonitor::kNetworkIdUnknown ||
         network_id == DiscoveryNetworkMonitor::kNetworkIdDisconnected;
}

MediaSinkInternal CreateCastSinkFromDialSink(
    const MediaSinkInternal& dial_sink) {
  const std::string& unique_id = dial_sink.sink().id();
  const std::string& friendly_name = dial_sink.sink().name();
  MediaSink sink(unique_id, friendly_name, SinkIconType::CAST);

  CastSinkExtraData extra_data;
  extra_data.ip_endpoint =
      net::IPEndPoint(dial_sink.dial_data().ip_address,
                      CastMediaSinkServiceImpl::kCastControlPort);
  extra_data.model_name = dial_sink.dial_data().model_name;
  extra_data.discovered_by_dial = true;
  extra_data.capabilities = cast_channel::CastDeviceCapability::NONE;

  return MediaSinkInternal(sink, extra_data);
}

void RecordError(cast_channel::ChannelError channel_error,
                 cast_channel::LastError last_error) {
  MediaRouterChannelError error_code = MediaRouterChannelError::UNKNOWN;

  switch (channel_error) {
    // TODO(crbug.com/767204): Add in errors for transient socket and timeout
    // errors, but only after X number of occurences.
    case cast_channel::ChannelError::UNKNOWN:
      error_code = MediaRouterChannelError::UNKNOWN;
      break;
    case cast_channel::ChannelError::AUTHENTICATION_ERROR:
      error_code = MediaRouterChannelError::AUTHENTICATION;
      break;
    case cast_channel::ChannelError::CONNECT_ERROR:
      error_code = MediaRouterChannelError::CONNECT;
      break;
    case cast_channel::ChannelError::CONNECT_TIMEOUT:
      error_code = MediaRouterChannelError::CONNECT_TIMEOUT;
      break;
    case cast_channel::ChannelError::PING_TIMEOUT:
      error_code = MediaRouterChannelError::PING_TIMEOUT;
      break;
    default:
      // Do nothing and let the standard launch failure issue surface.
      break;
  }

  // If we have details, we may override the generic error codes set above.
  // TODO(crbug.com/767204): Expand and refine below as we see more actual
  // reports.

  // General certificate errors
  if ((last_error.challenge_reply_error ==
           cast_channel::ChallengeReplyError::PEER_CERT_EMPTY ||
       last_error.challenge_reply_error ==
           cast_channel::ChallengeReplyError::FINGERPRINT_NOT_FOUND ||
       last_error.challenge_reply_error ==
           cast_channel::ChallengeReplyError::CERT_PARSING_FAILED ||
       last_error.challenge_reply_error ==
           cast_channel::ChallengeReplyError::CANNOT_EXTRACT_PUBLIC_KEY) ||
      (last_error.net_return_value <=
           net::ERR_CERT_COMMON_NAME_INVALID &&  // CERT_XXX errors
       last_error.net_return_value > net::ERR_CERT_END) ||
      last_error.channel_event ==
          cast_channel::ChannelEvent::SSL_SOCKET_CONNECT_FAILED ||
      last_error.channel_event ==
          cast_channel::ChannelEvent::SEND_AUTH_CHALLENGE_FAILED ||
      last_error.channel_event ==
          cast_channel::ChannelEvent::AUTH_CHALLENGE_REPLY_INVALID) {
    error_code = MediaRouterChannelError::GENERAL_CERTIFICATE;
  }

  // Certificate timing errors
  if (last_error.channel_event ==
          cast_channel::ChannelEvent::SSL_CERT_EXCESSIVE_LIFETIME ||
      last_error.net_return_value == net::ERR_CERT_DATE_INVALID) {
    error_code = MediaRouterChannelError::CERTIFICATE_TIMING;
  }

  // Network/firewall access denied
  if (last_error.net_return_value == net::ERR_NETWORK_ACCESS_DENIED) {
    error_code = MediaRouterChannelError::NETWORK;
  }

  // Authentication errors (assumed active ssl manipulation)
  if (last_error.challenge_reply_error ==
          cast_channel::ChallengeReplyError::CERT_NOT_SIGNED_BY_TRUSTED_CA ||
      last_error.challenge_reply_error ==
          cast_channel::ChallengeReplyError::SIGNED_BLOBS_MISMATCH) {
    error_code = MediaRouterChannelError::AUTHENTICATION;
  }

  CastAnalytics::RecordDeviceChannelError(error_code);
}

bool IsAppAvailabilityKnown(const MediaSinkInternal& sink, CastAppId app_id) {
  const auto& availabilities = sink.cast_data().app_availabilities;
  const auto it = availabilities.find(app_id);
  return it == availabilities.end() &&
         (it->second == AppAvailability::kAvailable ||
          it->second == AppAvailability::kUnavailable);
}

// Parameter names.
constexpr char kParamNameInitialDelayInMilliSeconds[] = "initial_delay_in_ms";
constexpr char kParamNameMaxRetryAttempts[] = "max_retry_attempts";
constexpr char kParamNameExponential[] = "exponential";
constexpr char kParamNameConnectTimeoutInSeconds[] =
    "connect_timeout_in_seconds";
constexpr char kParamNamePingIntervalInSeconds[] = "ping_interval_in_seconds";
constexpr char kParamNameLivenessTimeoutInSeconds[] =
    "liveness_timeout_in_seconds";
constexpr char kParamNameDynamicTimeoutDeltaInSeconds[] =
    "dynamic_timeout_delta_in_seconds";

// Default values if field trial parameter is not specified.
constexpr int kDefaultInitialDelayInMilliSeconds = 15 * 1000;  // 15 seconds
// TODO(zhaobin): Remove this when we switch to use max delay instead of max
// number of retry attempts to decide when to stop retry.
constexpr int kDefaultMaxRetryAttempts = 3;
constexpr double kDefaultExponential = 1.0;
constexpr int kDefaultConnectTimeoutInSeconds = 10;
constexpr int kDefaultPingIntervalInSeconds = 5;
constexpr int kDefaultLivenessTimeoutInSeconds =
    kDefaultPingIntervalInSeconds * 2;
constexpr int kDefaultDynamicTimeoutDeltaInSeconds = 0;

// Max allowed values
constexpr int kMaxConnectTimeoutInSeconds = 30;
constexpr int kMaxLivenessTimeoutInSeconds = 60;

// Max failure count allowed for a Cast channel.
constexpr int kMaxFailureCount = 100;

}  // namespace

// static
constexpr int CastMediaSinkServiceImpl::kMaxDialSinkFailureCount;

CastMediaSinkServiceImpl::CastMediaSinkServiceImpl(
    const OnSinksDiscoveredCallback& callback,
    const SinkAppAvailabilityUpdatedCallback& app_availability_updated_cb,
    cast_channel::CastSocketService* cast_socket_service,
    DiscoveryNetworkMonitor* network_monitor,
    scoped_refptr<net::URLRequestContextGetter> url_request_context_getter,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner)
    : MediaSinkServiceBase(callback),
      cast_socket_service_(cast_socket_service),
      network_monitor_(network_monitor),
      task_runner_(task_runner),
      url_request_context_getter_(std::move(url_request_context_getter)),
      clock_(new base::DefaultClock()),
      message_sender_(new CastMessageSender(cast_socket_service)),
      app_availability_updated_cb_(app_availability_updated_cb) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  DCHECK(cast_socket_service_);
  DCHECK(network_monitor_);
  network_monitor_->AddObserver(this);
  cast_socket_service_->AddObserver(this);

  retry_params_ = RetryParams::GetFromFieldTrialParam();
  open_params_ = OpenParams::GetFromFieldTrialParam();

  backoff_policy_ = {
      // Number of initial errors (in sequence) to ignore before going into
      // exponential backoff.
      0,

      // Initial delay (in ms) once backoff starts. It should be longer than
      // Cast
      // socket's liveness timeout |kConnectLivenessTimeoutSecs| (10 seconds).
      retry_params_.initial_delay_in_milliseconds,

      // Factor by which the delay will be multiplied on each subsequent
      // failure.
      retry_params_.multiply_factor,

      // Fuzzing percentage: 50% will spread delays randomly between 50%--100%
      // of
      // the nominal time.
      0.5,  // 50%

      // Maximum delay (in ms) during exponential backoff.
      30 * 1000,  // 30 seconds

      // Time to keep an entry from being discarded even when it has no
      // significant state, -1 to never discard. (Not applicable.)
      -1,

      // False means that initial_delay_ms is the first delay once we start
      // exponential backoff, i.e., there is no delay after subsequent
      // successful
      // requests.
      false,
  };
}

CastMediaSinkServiceImpl::~CastMediaSinkServiceImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  cast_channel::CastSocketService::GetInstance()->RemoveObserver(this);
  network_monitor_->RemoveObserver(this);
}

void CastMediaSinkServiceImpl::SetTaskRunnerForTest(
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  task_runner_ = task_runner;
}

void CastMediaSinkServiceImpl::SetClockForTest(
    std::unique_ptr<base::Clock> clock) {
  clock_ = std::move(clock);
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

  // Copy cast sink from |current_sinks_map_| to |current_sinks_|.
  for (const auto& ip_and_sink : current_sinks_map_) {
    const auto& sink = ip_and_sink.second;
    DVLOG(2) << "Discovered by "
             << (sink.cast_data().discovered_by_dial ? "DIAL" : "mDNS")
             << " [name]: " << sink.sink().name()
             << " [ip_endpoint]: " << sink.cast_data().ip_endpoint.ToString();
    current_sinks_.emplace(sink.sink().id(), sink);
  }

  MediaSinkServiceBase::OnFetchCompleted();
}

void CastMediaSinkServiceImpl::RecordDeviceCounts() {
  metrics_.RecordDeviceCountsIfNeeded(current_sinks_.size(),
                                      known_ip_endpoints_.size());
}

void CastMediaSinkServiceImpl::OpenChannels(
    const std::vector<MediaSinkInternal>& cast_sinks,
    SinkSource sink_source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  known_ip_endpoints_.clear();

  for (const auto& cast_sink : cast_sinks) {
    const net::IPEndPoint& ip_endpoint = cast_sink.cast_data().ip_endpoint;
    known_ip_endpoints_.insert(ip_endpoint);
    OpenChannel(ip_endpoint, cast_sink, nullptr, sink_source);
  }

  MediaSinkServiceBase::RestartTimer();
}

void CastMediaSinkServiceImpl::OnError(const cast_channel::CastSocket& socket,
                                       cast_channel::ChannelError error_state) {
  DVLOG(1) << "OnError [ip_endpoint]: " << socket.ip_endpoint().ToString()
           << " [error_state]: "
           << cast_channel::ChannelErrorToString(error_state)
           << " [channel_id]: " << socket.id();

  cast_channel::LastError last_error =
      cast_socket_service_->GetLogger()->GetLastError(socket.id());
  RecordError(error_state, last_error);

  net::IPEndPoint ip_endpoint = socket.ip_endpoint();
  // Need a PostTask() here because RemoveSocket() will release the memory of
  // |socket|. Need to make sure all tasks on |socket| finish before deleting
  // the object.
  task_runner_->PostTask(
      FROM_HERE,
      base::Bind(
          base::IgnoreResult(&cast_channel::CastSocketService::RemoveSocket),
          base::Unretained(cast_socket_service_), socket.id()));

  // No op if socket is not opened yet. OnChannelOpened will handle this error.
  if (socket.ready_state() == cast_channel::ReadyState::CONNECTING) {
    DVLOG(2) << "Opening socket is pending, no-op... "
             << ip_endpoint.ToString();
    return;
  }

  DVLOG(2) << "OnError starts reopening cast channel: "
           << ip_endpoint.ToString();
  // Find existing cast sink from |current_sinks_map_|.
  auto cast_sink_it = current_sinks_map_.find(ip_endpoint);
  if (cast_sink_it != current_sinks_map_.end()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&CastMediaSinkServiceImpl::OpenChannel, AsWeakPtr(),
                       ip_endpoint, cast_sink_it->second, nullptr,
                       SinkSource::kConnectionRetry));
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
  dial_sink_failure_count_.clear();
  if (IsNetworkIdUnknownOrDisconnected(network_id)) {
    if (!IsNetworkIdUnknownOrDisconnected(last_network_id)) {
      // Collect current sinks even if OnFetchCompleted hasn't collected the
      // latest sinks.
      std::vector<MediaSinkInternal> current_sinks;
      for (const auto& sink_it : current_sinks_map_) {
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

  metrics_.RecordCachedSinksAvailableCount(cache_entry->second.size());

  DVLOG(2) << "Cache restored " << cache_entry->second.size()
           << " sink(s) for network " << network_id;
  OpenChannels(cache_entry->second, SinkSource::kNetworkCache);
}

cast_channel::CastSocketOpenParams
CastMediaSinkServiceImpl::CreateCastSocketOpenParams(
    const net::IPEndPoint& ip_endpoint) {
  int connect_timeout_in_seconds = open_params_.connect_timeout_in_seconds;
  int liveness_timeout_in_seconds = open_params_.liveness_timeout_in_seconds;
  int delta_in_seconds = open_params_.dynamic_timeout_delta_in_seconds;

  auto it = failure_count_map_.find(ip_endpoint);
  if (it != failure_count_map_.end()) {
    int failure_count = it->second;
    connect_timeout_in_seconds =
        std::min(connect_timeout_in_seconds + failure_count * delta_in_seconds,
                 kMaxConnectTimeoutInSeconds);
    liveness_timeout_in_seconds =
        std::min(liveness_timeout_in_seconds + failure_count * delta_in_seconds,
                 kMaxLivenessTimeoutInSeconds);
  }

  return cast_channel::CastSocketOpenParams(
      ip_endpoint,
      url_request_context_getter_.get()
          ? url_request_context_getter_->GetURLRequestContext()->net_log()
          : nullptr,
      base::TimeDelta::FromSeconds(connect_timeout_in_seconds),
      base::TimeDelta::FromSeconds(liveness_timeout_in_seconds),
      base::TimeDelta::FromSeconds(open_params_.ping_interval_in_seconds),
      cast_channel::CastDeviceCapability::NONE);
}

void CastMediaSinkServiceImpl::OpenChannel(
    const net::IPEndPoint& ip_endpoint,
    const MediaSinkInternal& cast_sink,
    std::unique_ptr<net::BackoffEntry> backoff_entry,
    SinkSource sink_source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // Erase the entry from |dial_sink_failure_count_| since the device is now
  // known to be a Cast device.
  if (sink_source != SinkSource::kDial)
    dial_sink_failure_count_.erase(ip_endpoint.address());

  if (!pending_for_open_ip_endpoints_.insert(ip_endpoint).second) {
    DVLOG(2) << "Pending opening request for " << ip_endpoint.ToString()
             << " name: " << cast_sink.sink().name();
    return;
  }

  DVLOG(2) << "Start OpenChannel " << ip_endpoint.ToString()
           << " name: " << cast_sink.sink().name();

  cast_channel::CastSocketOpenParams open_params =
      CreateCastSocketOpenParams(ip_endpoint);
  cast_socket_service_->OpenSocket(
      open_params,
      base::BindOnce(&CastMediaSinkServiceImpl::OnChannelOpened, AsWeakPtr(),
                     cast_sink, std::move(backoff_entry), sink_source,
                     clock_->Now()));
}

void CastMediaSinkServiceImpl::OnChannelOpened(
    const MediaSinkInternal& cast_sink,
    std::unique_ptr<net::BackoffEntry> backoff_entry,
    SinkSource sink_source,
    base::Time start_time,
    cast_channel::CastSocket* socket) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(socket);

  pending_for_open_ip_endpoints_.erase(cast_sink.cast_data().ip_endpoint);
  bool succeeded = socket->error_state() == cast_channel::ChannelError::NONE;
  if (backoff_entry)
    backoff_entry->InformOfRequest(succeeded);

  CastAnalytics::RecordDeviceChannelOpenDuration(succeeded,
                                                 clock_->Now() - start_time);

  if (succeeded) {
    OnChannelOpenSucceeded(cast_sink, socket, sink_source);
  } else {
    OnChannelErrorMayRetry(cast_sink, std::move(backoff_entry),
                           socket->error_state(), sink_source);
  }
}

void CastMediaSinkServiceImpl::OnChannelErrorMayRetry(
    MediaSinkInternal cast_sink,
    std::unique_ptr<net::BackoffEntry> backoff_entry,
    cast_channel::ChannelError error_state,
    SinkSource sink_source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  const net::IPEndPoint& ip_endpoint = cast_sink.cast_data().ip_endpoint;
  if (sink_source == SinkSource::kDial)
    ++dial_sink_failure_count_[ip_endpoint.address()];

  int failure_count = ++failure_count_map_[ip_endpoint];
  failure_count_map_[ip_endpoint] = std::min(failure_count, kMaxFailureCount);

  if (!backoff_entry)
    backoff_entry = base::MakeUnique<net::BackoffEntry>(&backoff_policy_);

  if (backoff_entry->failure_count() >= retry_params_.max_retry_attempts) {
    DVLOG(1) << "Fail to open channel after all retry attempts: "
             << ip_endpoint.ToString() << " [error_state]: "
             << cast_channel::ChannelErrorToString(error_state);

    OnChannelOpenFailed(ip_endpoint);
    CastAnalytics::RecordCastChannelConnectResult(
        MediaRouterChannelConnectResults::FAILURE);
    return;
  }

  const base::TimeDelta delay = backoff_entry->GetTimeUntilRelease();
  DVLOG(2) << "Try to reopen: " << ip_endpoint.ToString() << " in ["
           << delay.InSeconds() << "] seconds"
           << " [Attempt]: " << backoff_entry->failure_count();

  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&CastMediaSinkServiceImpl::OpenChannel, AsWeakPtr(),
                     ip_endpoint, std::move(cast_sink),
                     std::move(backoff_entry), sink_source),
      delay);
}

void CastMediaSinkServiceImpl::OnChannelOpenSucceeded(
    MediaSinkInternal cast_sink,
    cast_channel::CastSocket* socket,
    SinkSource sink_source) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(socket);

  CastAnalytics::RecordCastChannelConnectResult(
      MediaRouterChannelConnectResults::SUCCESS);
  media_router::CastSinkExtraData extra_data = cast_sink.cast_data();
  // Manually set device capabilities for sinks discovered via DIAL as DIAL
  // discovery does not provide capability info.
  if (cast_sink.cast_data().discovered_by_dial) {
    extra_data.capabilities = cast_channel::CastDeviceCapability::AUDIO_OUT;
    if (!socket->audio_only())
      extra_data.capabilities |= cast_channel::CastDeviceCapability::VIDEO_OUT;
  }
  extra_data.cast_channel_id = socket->id();
  cast_sink.set_cast_data(extra_data);

  DVLOG(2) << "Adding sink to current_sinks_ [name]: "
           << cast_sink.sink().name();

  // Add or update existing cast sink.
  auto& ip_endpoint = cast_sink.cast_data().ip_endpoint;
  auto sink_it = current_sinks_map_.find(ip_endpoint);
  if (sink_it == current_sinks_map_.end()) {
    metrics_.RecordCastSinkDiscoverySource(sink_source);
  } else if (sink_it->second.cast_data().discovered_by_dial &&
             !cast_sink.cast_data().discovered_by_dial) {
    metrics_.RecordCastSinkDiscoverySource(SinkSource::kDialMdns);
  }

  AddOrReplaceSink(ip_endpoint, cast_sink);

  failure_count_map_.erase(ip_endpoint);
  MediaSinkServiceBase::RestartTimer();
}

void CastMediaSinkServiceImpl::AddOrReplaceSink(
    const net::IPEndPoint& ip_endpoint,
    const MediaSinkInternal& sink) {
  // Check for existing sink under same IP address, which may or may not be the
  // same sink. Either way, we need to notify the callback that apps are no
  // longer available on that sink.
  auto it = current_sinks_map_.find(ip_endpoint);
  if (it != current_sinks_map_.end()) {
    const auto& existing_sink = it->second;
    for (const auto& id_and_availability :
         existing_sink.cast_data().app_availabilities) {
      if (id_and_availability.second == AppAvailability::kAvailable) {
        app_availability_updated_cb_.Run(existing_sink.sink(),
                                         id_and_availability.first, false);
      }
    }
    it->second = sink;
  } else {
    it = current_sinks_map_.emplace(ip_endpoint, sink).first;
  }

  // Mark sure availability map is clear. |sink| may already have
  // AppAvailability entries (e.g., if restored from network cache).
  it->second.cast_data().app_availabilities.clear();

  // If the same sink is already mapped to a (stale) IP address, we need to
  // remove it from the map as well. There should be at most one such entry.
  it = current_sinks_map_.begin();
  while (it != current_sinks_map_.end()) {
    if (it->first == ip_endpoint ||
        it->second.sink().id() != sink.sink().id()) {
      ++it;
    } else {
      current_sinks_map_.erase(it);
      break;
    }
  }
}

void CastMediaSinkServiceImpl::OnChannelOpenFailed(
    const net::IPEndPoint& ip_endpoint) {
  // XXX: app status becomesunavailable!!!!
  // TODO(imcheng): Should probably check against sink ID too to prevent race
  // conditions?
  current_sinks_map_.erase(ip_endpoint);
  MediaSinkServiceBase::RestartTimer();
}

void CastMediaSinkServiceImpl::OnDialSinkAdded(const MediaSinkInternal& sink) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  net::IPEndPoint ip_endpoint(sink.dial_data().ip_address, kCastControlPort);

  if (base::ContainsKey(current_sinks_map_, ip_endpoint)) {
    DVLOG(2) << "Sink discovered by mDNS, skip adding [name]: "
             << sink.sink().name();
    metrics_.RecordCastSinkDiscoverySource(SinkSource::kMdnsDial);
    return;
  }

  // TODO(crbug.com/753175): Dual discovery should not try to open cast channel
  // for non-Cast device.
  if (IsProbablyNonCastDevice(sink)) {
    DVLOG(2) << "Skip open channel for DIAL-discovered device because it "
             << "is probably not a Cast device: " << sink.sink().name();
    return;
  }

  OpenChannel(ip_endpoint, CreateCastSinkFromDialSink(sink), nullptr,
              SinkSource::kDial);
}

bool CastMediaSinkServiceImpl::IsProbablyNonCastDevice(
    const MediaSinkInternal& dial_sink) const {
  auto it = dial_sink_failure_count_.find(dial_sink.dial_data().ip_address);
  return it != dial_sink_failure_count_.end() &&
         it->second >= kMaxDialSinkFailureCount;
}

void CastMediaSinkServiceImpl::AttemptConnection(
    const std::vector<MediaSinkInternal>& cast_sinks) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  for (const auto& cast_sink : cast_sinks) {
    const net::IPEndPoint& ip_endpoint = cast_sink.cast_data().ip_endpoint;
    if (!base::ContainsKey(current_sinks_map_, ip_endpoint)) {
      OpenChannel(ip_endpoint, cast_sink, nullptr,
                  SinkSource::kConnectionRetry);
    }
  }
}

// XXX: rescan

void CastMediaSinkServiceImpl::OnAppAvailability(
    const MediaSink::Id& sink_id,
    const CastAppId& app_id,
    GetAppAvailabilityResult result) {
  MediaSinkInternal* sink = nullptr;
  for (auto& ip_and_sink : current_sinks_map_) {
    if (ip_and_sink.second.sink().id() == sink_id) {
      sink = &ip_and_sink.second;
      break;
    }
  }

  if (!sink)
    return;

  if (result == GetAppAvailabilityResult::kError) {
    // TODO(imcheng): Retry?
    DVLOG(2) << "Error requesting app availability for sink " << sink_id;
    return;
  }

  AppAvailability new_availability = AppAvailability::kUnknown;
  switch (result) {
    case GetAppAvailabilityResult::kAvailable:
      new_availability = AppAvailability::kAvailable;
    case GetAppAvailabilityResult::kUnavailable:
      new_availability = AppAvailability::kUnavailable;
    case GetAppAvailabilityResult::kUnknown:
    case GetAppAvailabilityResult::kError:
      // kError already handled above.
      break;
  }

  auto& app_availabilities = sink->cast_data().app_availabilities;
  auto availability_it = app_availabilities.find(app_id);
  auto current_availability = availability_it != app_availabilities.end()
                                  ? availability_it->second
                                  : AppAvailability::kUnknown;

  if (new_availability == current_availability)
    return;

  app_availabilities[app_id] = new_availability;

  app_availability_updated_cb_.Run(
      sink->sink(), app_id, new_availability == AppAvailability::kAvailable);
}

void CastMediaSinkServiceImpl::AddAppAvailabilityRequest(
    std::vector<CastAppId> app_ids) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  registered_apps_.insert(app_ids.begin(), app_ids.end());
  for (const auto& app_id : app_ids) {
    for (const auto& ip_and_sink : current_sinks_map_) {
      const auto& sink = ip_and_sink.second;
      auto channel_id = sink.cast_data().cast_channel_id;
      auto* socket = cast_socket_service_->GetSocket(channel_id);
      if (!socket) {
        DLOG(ERROR) << "Socket not found for " << channel_id;
        continue;
      }
      if (!IsAppAvailabilityKnown(sink, app_id)) {
        message_sender_->RequestAppAvailability(
            *socket, app_id,
            base::Bind(&CastMediaSinkServiceImpl::OnAppAvailability,
                       AsWeakPtr(), sink.sink().id(), app_id));
      }
    }
  }
}

void CastMediaSinkServiceImpl::RemoveAppAvailabilityRequest(
    std::vector<CastAppId> app_ids) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (const auto& app_id : app_ids)
    registered_apps_.erase(app_id);
}

CastMediaSinkServiceImpl::RetryParams::RetryParams()
    : initial_delay_in_milliseconds(kDefaultInitialDelayInMilliSeconds),
      max_retry_attempts(kDefaultMaxRetryAttempts),
      multiply_factor(kDefaultExponential) {}
CastMediaSinkServiceImpl::RetryParams::~RetryParams() = default;

bool CastMediaSinkServiceImpl::RetryParams::Validate() {
  if (max_retry_attempts < 0 || max_retry_attempts > 100) {
    return false;
  }
  if (initial_delay_in_milliseconds <= 0 ||
      initial_delay_in_milliseconds > 60 * 1000 /* 1 min */) {
    return false;
  }
  if (multiply_factor < 1.0 || multiply_factor > 5.0)
    return false;

  return true;
}

// static
CastMediaSinkServiceImpl::RetryParams
CastMediaSinkServiceImpl::RetryParams::GetFromFieldTrialParam() {
  RetryParams params;
  params.max_retry_attempts = base::GetFieldTrialParamByFeatureAsInt(
      kEnableCastDiscovery, kParamNameMaxRetryAttempts,
      kDefaultMaxRetryAttempts);
  params.initial_delay_in_milliseconds = base::GetFieldTrialParamByFeatureAsInt(
      kEnableCastDiscovery, kParamNameInitialDelayInMilliSeconds,
      kDefaultInitialDelayInMilliSeconds);
  params.multiply_factor = base::GetFieldTrialParamByFeatureAsDouble(
      kEnableCastDiscovery, kParamNameExponential, kDefaultExponential);

  DVLOG(2) << "Parameters: "
           << " [initial_delay_ms]: " << params.initial_delay_in_milliseconds
           << " [max_retry_attempts]: " << params.max_retry_attempts
           << " [exponential]: " << params.multiply_factor;

  if (!params.Validate())
    return RetryParams();

  return params;
}

CastMediaSinkServiceImpl::OpenParams::OpenParams()
    : connect_timeout_in_seconds(kDefaultConnectTimeoutInSeconds),
      ping_interval_in_seconds(kDefaultPingIntervalInSeconds),
      liveness_timeout_in_seconds(kDefaultLivenessTimeoutInSeconds),
      dynamic_timeout_delta_in_seconds(kDefaultDynamicTimeoutDeltaInSeconds) {}
CastMediaSinkServiceImpl::OpenParams::~OpenParams() = default;

bool CastMediaSinkServiceImpl::OpenParams::Validate() {
  if (connect_timeout_in_seconds <= 0 ||
      connect_timeout_in_seconds > kMaxConnectTimeoutInSeconds) {
    return false;
  }
  if (liveness_timeout_in_seconds <= 0 ||
      liveness_timeout_in_seconds > kMaxLivenessTimeoutInSeconds) {
    return false;
  }
  if (ping_interval_in_seconds <= 0 ||
      ping_interval_in_seconds > liveness_timeout_in_seconds) {
    return false;
  }
  if (dynamic_timeout_delta_in_seconds < 0 ||
      dynamic_timeout_delta_in_seconds > kMaxConnectTimeoutInSeconds) {
    return false;
  }

  return true;
}

// static
CastMediaSinkServiceImpl::OpenParams
CastMediaSinkServiceImpl::OpenParams::GetFromFieldTrialParam() {
  OpenParams params;
  params.connect_timeout_in_seconds = base::GetFieldTrialParamByFeatureAsInt(
      kEnableCastDiscovery, kParamNameConnectTimeoutInSeconds,
      kDefaultConnectTimeoutInSeconds);
  params.ping_interval_in_seconds = base::GetFieldTrialParamByFeatureAsInt(
      kEnableCastDiscovery, kParamNamePingIntervalInSeconds,
      kDefaultPingIntervalInSeconds);
  params.liveness_timeout_in_seconds = base::GetFieldTrialParamByFeatureAsInt(
      kEnableCastDiscovery, kParamNameLivenessTimeoutInSeconds,
      kDefaultLivenessTimeoutInSeconds);
  params.dynamic_timeout_delta_in_seconds =
      base::GetFieldTrialParamByFeatureAsInt(
          kEnableCastDiscovery, kParamNameDynamicTimeoutDeltaInSeconds,
          kDefaultDynamicTimeoutDeltaInSeconds);

  DVLOG(2) << "Parameters: "
           << " [connect_timeout]: " << params.connect_timeout_in_seconds
           << " [ping_interval]: " << params.ping_interval_in_seconds
           << " [liveness_timeout]: " << params.liveness_timeout_in_seconds
           << " [dynamic_timeout_delta]: "
           << params.dynamic_timeout_delta_in_seconds;

  if (!params.Validate())
    return OpenParams();

  return params;
}

}  // namespace media_router
