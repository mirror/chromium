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

using content::BrowserThread;

namespace media_router {

DialMediaSinkServiceImpl::DialMediaSinkServiceImpl(
    const OnSinksDiscoveredCallback& callback,
    const OnAvailableSinksUpdatedCallback& available_sinks_updated_callback,
    net::URLRequestContextGetter* request_context)
    : MediaSinkServiceBase(callback),
      available_sinks_updated_callback_(available_sinks_updated_callback),
      observer_(nullptr),
      request_context_(request_context) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(request_context_);

  app_discovery_service_.reset(new DialAppDiscoveryService(
      base::Bind(&DialMediaSinkServiceImpl::OnDialAppInfoAvailable,
                 base::Unretained(this)),
      base::Bind(&DialMediaSinkServiceImpl::OnDialAppInfoError,
                 base::Unretained(this))));
}

DialMediaSinkServiceImpl::~DialMediaSinkServiceImpl() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  Stop();
}

void DialMediaSinkServiceImpl::Start() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
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

void DialMediaSinkServiceImpl::Stop() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!dial_registry_)
    return;

  dial_registry_->OnListenerRemoved();
  dial_registry_->UnregisterObserver(this);
  dial_registry_ = nullptr;
  MediaSinkServiceBase::StopTimer();
}

void DialMediaSinkServiceImpl::OnUserGesture() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // Re-sync sinks to CastMediaSinkService. It's possible that a DIAL-discovered
  // sink was added to CastMediaSinkService earlier, but was removed due to
  // flaky network. This gives CastMediaSinkService an opportunity to recover
  // even if mDNS is not working for some reason.
  DVLOG(2) << "OnUserGesture: re-syncing " << current_sinks_.size()
           << " sinks to CastMediaSinkService";
  if (observer_) {
    for (const auto& sink : current_sinks_)
      observer_->OnDialSinkAdded(sink);
  }

  // Scan existing sinks for all registered apps.
  for (const auto& dial_sink : current_sinks_) {
    for (const auto& app_name_it : app_name_sink_id_map_) {
      app_discovery_service_->FetchDialAppInfo(dial_sink.dial_data().app_url,
                                               app_name_it.first,
                                               request_context_.get());
    }
  }
}

void DialMediaSinkServiceImpl::StartMonitoringAvailableSinksForApp(
    const std::string& app_name) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!base::ContainsKey(app_name_sink_id_map_, app_name)) {
    app_name_sink_id_map_.insert(
        std::make_pair(app_name, std::set<std::string>()));
  }

  // Start checking if |app_name| is available on existing sinks.
  for (const auto& dial_sink : current_sinks_) {
    app_discovery_service_->FetchDialAppInfo(dial_sink.dial_data().app_url,
                                             app_name, request_context_.get());
  }
}

void DialMediaSinkServiceImpl::StopMonitoringAvailableSinksForApp(
    const std::string& app_name) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  app_name_sink_id_map_.erase(app_name);
}

DeviceDescriptionService* DialMediaSinkServiceImpl::GetDescriptionService() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  if (!description_service_.get()) {
    description_service_.reset(new DeviceDescriptionService(
        base::Bind(&DialMediaSinkServiceImpl::OnDeviceDescriptionAvailable,
                   base::Unretained(this)),
        base::Bind(&DialMediaSinkServiceImpl::OnDeviceDescriptionError,
                   base::Unretained(this))));
  }
  return description_service_.get();
}

void DialMediaSinkServiceImpl::SetObserver(
    DialMediaSinkServiceObserver* observer) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  observer_ = observer;
}

void DialMediaSinkServiceImpl::ClearObserver() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  observer_ = nullptr;
}

void DialMediaSinkServiceImpl::SetDialRegistryForTest(
    DialRegistry* dial_registry) {
  DCHECK(!test_dial_registry_);
  test_dial_registry_ = dial_registry;
}

void DialMediaSinkServiceImpl::SetDescriptionServiceForTest(
    std::unique_ptr<DeviceDescriptionService> description_service) {
  DCHECK(!description_service_);
  description_service_ = std::move(description_service);
}

void DialMediaSinkServiceImpl::OnDialDeviceEvent(
    const DialRegistry::DeviceList& devices) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DVLOG(2) << "DialMediaSinkServiceImpl::OnDialDeviceEvent found "
           << devices.size() << " devices";

  current_sinks_.clear();
  current_devices_ = devices;

  GetDescriptionService()->GetDeviceDescriptions(devices,
                                                 request_context_.get());
}

void DialMediaSinkServiceImpl::OnDialError(DialRegistry::DialErrorCode type) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DVLOG(2) << "OnDialError [DialErrorCode]: " << static_cast<int>(type);
}

void DialMediaSinkServiceImpl::OnDeviceDescriptionAvailable(
    const DialDeviceData& device_data,
    const ParsedDialDeviceDescription& description_data) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!base::ContainsValue(current_devices_, device_data)) {
    DVLOG(2) << "Device data not found in current device data list...";
    return;
  }

  // When use this "sink" within browser, please note it will have a different
  // ID when it is sent to the extension, because it derives a different sink ID
  // using the given sink ID.
  MediaSink sink(description_data.unique_id, description_data.friendly_name,
                 SinkIconType::GENERIC);
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
  if (observer_)
    observer_->OnDialSinkAdded(dial_sink);

  // Start checking if all registered apps are available on |dial_sink|.
  for (const auto& app_name_it : app_name_sink_id_map_) {
    app_discovery_service_->FetchDialAppInfo(dial_sink.dial_data().app_url,
                                             app_name_it.first,
                                             request_context_.get());
  }
  // Start fetch timer again if device description comes back after
  // |finish_timer_| fires.
  MediaSinkServiceBase::RestartTimer();
}

void DialMediaSinkServiceImpl::OnDeviceDescriptionError(
    const DialDeviceData& device,
    const std::string& error_message) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DVLOG(2) << "OnDescriptionFetchesError [message]: " << error_message;
}

void DialMediaSinkServiceImpl::OnDialAppInfoAvailable(
    const GURL& app_url,
    const ParsedDialAppInfo& app_info) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  bool is_available = (app_info.app_status == DialAppStatus::AVAILABLE);
  OnDialAppInfoUpdated(app_url, is_available);
}

void DialMediaSinkServiceImpl::OnDialAppInfoError(
    const GURL& app_url,
    const std::string& error_message) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DVLOG(1) << "OnDialAppInfoError [app_url]: " << app_url.spec()
           << " [error_message]: " << error_message;
  OnDialAppInfoUpdated(app_url, false /* is_available */);
}

void DialMediaSinkServiceImpl::OnDialAppInfoUpdated(const GURL& app_url,
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

  auto sink_it =
      std::find_if(current_sinks_.begin(), current_sinks_.end(),
                   [&partial_app_url](const MediaSinkInternal& sink) {
                     return sink.dial_data().app_url == partial_app_url;
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
  std::vector<MediaSink> sinks;
  for (const auto& sink : current_sinks_) {
    MediaSink media_sink = sink.sink();
    if (available_sink_id_set.find(media_sink.id()) !=
        available_sink_id_set.end()) {
      sinks.push_back(media_sink);
    }
  }

  available_sinks_updated_callback_.Run(app_name, sinks);
}

void DialMediaSinkServiceImpl::RecordDeviceCounts() {
  metrics_.RecordDeviceCountsIfNeeded(current_sinks_.size(),
                                      current_devices_.size());
}

}  // namespace media_router
