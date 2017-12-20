// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/dial/dial_media_sink_service_impl.h"

#include <algorithm>

#include "chrome/browser/media/router/discovery/dial/dial_device_data.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/service_manager/public/cpp/connector.h"

using content::BrowserThread;

namespace media_router {

DialMediaSinkServiceImpl::DialMediaSinkServiceImpl(
    std::unique_ptr<service_manager::Connector> connector,
    const OnSinksDiscoveredCallback& on_sinks_discovered_cb,
    const OnDialSinkAddedCallback& dial_sink_added_cb,
    const OnAvailableSinksUpdatedCallback& available_sinks_updated_callback,
    const scoped_refptr<net::URLRequestContextGetter>& request_context,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner)
    : MediaSinkServiceBase(on_sinks_discovered_cb),
      connector_(std::move(connector)),
      description_service_(std::make_unique<DeviceDescriptionService>(
          connector_.get(),
          base::Bind(&DialMediaSinkServiceImpl::OnDeviceDescriptionAvailable,
                     base::Unretained(this)),
          base::Bind(&DialMediaSinkServiceImpl::OnDeviceDescriptionError,
                     base::Unretained(this)))),
      dial_sink_added_cb_(dial_sink_added_cb),
      available_sinks_updated_callback_(available_sinks_updated_callback),
      request_context_(request_context),
      task_runner_(task_runner) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
  DCHECK(request_context_);

  app_discovery_service_.reset(new DialAppDiscoveryService(
      connector_.get(),
      base::BindRepeating(&DialMediaSinkServiceImpl::OnAppInfoAvailable,
                          base::Unretained(this)),
      base::BindRepeating(&DialMediaSinkServiceImpl::OnAppInfoError,
                          base::Unretained(this))));
}

DialMediaSinkServiceImpl::~DialMediaSinkServiceImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (dial_registry_) {
    dial_registry_->OnListenerRemoved();
    dial_registry_->UnregisterObserver(this);
    dial_registry_ = nullptr;
  }
}

void DialMediaSinkServiceImpl::Start() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (dial_registry_)
    return;

  dial_registry_ =
      test_dial_registry_ ? test_dial_registry_ : DialRegistry::GetInstance();
  dial_registry_->SetNetLog(
      request_context_->GetURLRequestContext()->net_log());
  dial_registry_->RegisterObserver(this);
  dial_registry_->OnListenerAdded();
  MediaSinkServiceBase::StartTimer();
}

void DialMediaSinkServiceImpl::OnUserGesture() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // Re-sync sinks to CastMediaSinkService. It's possible that a DIAL-discovered
  // sink was added to CastMediaSinkService earlier, but was removed due to
  // flaky network. This gives CastMediaSinkService an opportunity to recover
  // even if mDNS is not working for some reason.
  DVLOG(2) << "OnUserGesture: re-syncing " << current_sinks_.size()
           << " sinks to CastMediaSinkService";

  if (dial_sink_added_cb_) {
    for (const auto& sink : current_sinks_)
      dial_sink_added_cb_.Run(sink);
  }

  // Scan existing sinks for all registered apps.
  for (const auto& dial_sink : current_sinks_) {
    for (const auto& app_name_it : app_name_sink_id_map_) {
      FetchAppInfoForSink(app_name_it.first, dial_sink, true /* force_fetch */);
    }
  }
}

void DialMediaSinkServiceImpl::StartMonitoringAvailableSinksForApp(
    const std::string& app_name) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (base::ContainsKey(app_name_sink_id_map_, app_name))
    return;

  app_name_sink_id_map_.insert(
      std::make_pair(app_name, std::set<std::string>()));
  // Start checking if |app_name| is available on existing sinks.
  for (const auto& dial_sink : current_sinks_)
    FetchAppInfoForSink(app_name, dial_sink, false /* force_fetch */);
}

void DialMediaSinkServiceImpl::StopMonitoringAvailableSinksForApp(
    const std::string& app_name) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  app_name_sink_id_map_.erase(app_name);
}

void DialMediaSinkServiceImpl::SetDialRegistryForTest(
    DialRegistry* dial_registry) {
  DCHECK(!test_dial_registry_);
  test_dial_registry_ = dial_registry;
}

void DialMediaSinkServiceImpl::SetDescriptionServiceForTest(
    std::unique_ptr<DeviceDescriptionService> description_service) {
  description_service_ = std::move(description_service);
}

void DialMediaSinkServiceImpl::SetAppDiscoveryServiceForTest(
    std::unique_ptr<DialAppDiscoveryService> app_discovery_service) {
  app_discovery_service_ = std::move(app_discovery_service);
}

void DialMediaSinkServiceImpl::OnDialDeviceEvent(
    const DialRegistry::DeviceList& devices) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(2) << "DialMediaSinkServiceImpl::OnDialDeviceEvent found "
           << devices.size() << " devices";

  current_sinks_.clear();
  current_devices_ = devices;

  description_service_->GetDeviceDescriptions(devices, request_context_.get());
}

void DialMediaSinkServiceImpl::OnDialError(DialRegistry::DialErrorCode type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(2) << "OnDialError [DialErrorCode]: " << static_cast<int>(type);
}

void DialMediaSinkServiceImpl::OnDeviceDescriptionAvailable(
    const DialDeviceData& device_data,
    const ParsedDialDeviceDescription& description_data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!base::ContainsValue(current_devices_, device_data)) {
    DVLOG(2) << "Device data not found in current device data list...";
    return;
  }

  // When use this "sink" within browser, please note it will have a different
  // ID when it is sent to the extension, because it derives a different sink ID
  // using the given sink ID.
  MediaSink sink(description_data.unique_id, description_data.friendly_name,
                 SinkIconType::GENERIC, MediaRouteProviderId::EXTENSION);
  DialSinkExtraData extra_data;
  extra_data.app_url = description_data.app_url;
  extra_data.model_name = description_data.model_name;
  std::string ip_address = device_data.device_description_url().host();
  if (!extra_data.ip_address.AssignFromIPLiteral(ip_address)) {
    DVLOG(1) << "Invalid ip_address: " << ip_address;
    return;
  }

  MediaSinkInternal dial_sink(sink, extra_data);
  current_sinks_.insert(dial_sink);
  if (dial_sink_added_cb_)
    dial_sink_added_cb_.Run(dial_sink);

  // Start checking if all registered apps are available on |dial_sink|.
  for (const auto& app_name_it : app_name_sink_id_map_)
    FetchAppInfoForSink(app_name_it.first, dial_sink, false /* force_fetch */);

  // Start fetch timer again if device description comes back after
  // |finish_timer_| fires.
  MediaSinkServiceBase::RestartTimer();
}

void DialMediaSinkServiceImpl::OnDeviceDescriptionError(
    const DialDeviceData& device,
    const std::string& error_message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(2) << "OnDeviceDescriptionError [message]: " << error_message;
}

void DialMediaSinkServiceImpl::OnAppInfoAvailable(const GURL& app_url,
                                                  const DialAppInfo& app_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  bool is_available = (app_info.app_status == DialAppStatus::kAvailable);
  OnAppInfoUpdated(app_url, is_available);
}

void DialMediaSinkServiceImpl::OnAppInfoError(
    const GURL& app_url,
    const std::string& error_message) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DVLOG(1) << "OnAppInfoError [app_url]: " << app_url.spec()
           << " [error_message]: " << error_message;
  OnAppInfoUpdated(app_url, false /* is_available */);
}

void DialMediaSinkServiceImpl::OnAppInfoUpdated(const GURL& app_url,
                                                bool is_available) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  GURL partial_app_url = app_url.GetWithoutFilename();
  std::string app_name = app_url.ExtractFileName();

  auto sinks_map_it = app_name_sink_id_map_.find(app_name);
  if (sinks_map_it == app_name_sink_id_map_.end()) {
    DVLOG(2) << "App name not registered: " << app_name;
    return;
  }
  auto& available_sink_id_set = sinks_map_it->second;

  auto sink_it = std::find_if(
      current_sinks_.begin(), current_sinks_.end(),
      [&partial_app_url](const MediaSinkInternal& sink) {
        return sink.dial_data().app_url.host() == partial_app_url.host();
      });
  if (sink_it == current_sinks_.end()) {
    DVLOG(2) << "No sink found for app url " << app_url.spec();
    return;
  }
  std::string sink_id = sink_it->sink().id();

  if (is_available) {
    if (available_sink_id_set.insert(sink_id).second)
      NotifySinkObservers(app_name, available_sink_id_set);
  } else {
    if (available_sink_id_set.erase(sink_id) > 0)
      NotifySinkObservers(app_name, available_sink_id_set);
  }
}

void DialMediaSinkServiceImpl::NotifySinkObservers(
    const std::string& app_name,
    const std::set<std::string>& available_sink_id_set) {
  std::vector<MediaSinkInternal> sinks;
  for (const auto& sink : current_sinks_) {
    if (available_sink_id_set.find(sink.sink().id()) !=
        available_sink_id_set.end()) {
      sinks.push_back(sink);
    }
  }

  available_sinks_updated_callback_.Run(app_name, sinks);
}

void DialMediaSinkServiceImpl::FetchAppInfoForSink(
    const std::string& app_name,
    const MediaSinkInternal& dial_sink,
    bool force_fetch) {
  // If |partial_app_url| is http://127.0.0.1/apps, convert it to
  // http://127.0.0.1/apps/
  GURL partial_app_url = dial_sink.dial_data().app_url;
  std::string possible_slash =
      partial_app_url.ExtractFileName().empty() ? "" : "/";
  std::string str_app_url = partial_app_url.spec() + possible_slash + app_name;

  app_discovery_service_->FetchDialAppInfo(GURL(str_app_url),
                                           request_context_.get(), force_fetch);
}

void DialMediaSinkServiceImpl::RecordDeviceCounts() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  metrics_.RecordDeviceCountsIfNeeded(current_sinks_.size(),
                                      current_devices_.size());
}

}  // namespace media_router
