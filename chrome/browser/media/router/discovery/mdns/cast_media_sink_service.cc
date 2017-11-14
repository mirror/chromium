// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/media/router/discovery/discovery_network_monitor.h"
#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service_impl.h"
#include "chrome/browser/media/router/discovery/mdns/dns_sd_delegate.h"
#include "chrome/browser/media/router/discovery/mdns/dns_sd_registry.h"
#include "chrome/browser/media/router/media_sinks_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/media_router/media_sink.h"
#include "chrome/common/media_router/media_source_helper.h"
#include "components/cast_channel/cast_socket_service.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_address.h"
#include "net/url_request/url_request_context_getter.h"

namespace media_router {

namespace {

enum ErrorType {
  NONE,
  NOT_CAST_DEVICE,
  MISSING_ID,
  MISSING_FRIENDLY_NAME,
  MISSING_OR_INVALID_IP_ADDRESS,
  MISSING_OR_INVALID_PORT,
};

ErrorType CreateCastMediaSink(const DnsSdService& service,
                              MediaSinkInternal* cast_sink) {
  DCHECK(cast_sink);
  if (service.service_name.find(CastMediaSinkService::kCastServiceType) ==
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
  MediaSink sink(unique_id, friendly_name, SinkIconType::CAST);

  CastSinkExtraData extra_data;
  extra_data.ip_endpoint =
      net::IPEndPoint(ip_address, service.service_host_port.port());
  extra_data.model_name = service_data["md"];
  extra_data.capabilities = cast_channel::CastDeviceCapability::NONE;

  unsigned capacities;
  if (base::StringToUint(service_data["ca"], &capacities))
    extra_data.capabilities = capacities;

  cast_sink->set_sink(sink);
  cast_sink->set_cast_data(extra_data);

  return ErrorType::NONE;
}

}  // namespace

// static
const char CastMediaSinkService::kCastServiceType[] = "_googlecast._tcp.local";

CastMediaSinkService::CastMediaSinkService(
    const OnSinksDiscoveredCallback& callback,
    content::BrowserContext* browser_context,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner)
    : MediaSinkService(callback),
      task_runner_(task_runner),
      browser_context_(browser_context) {
  // TODO(crbug.com/749305): Migrate the discovery code to use sequences.
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(task_runner_);
}

CastMediaSinkService::CastMediaSinkService(
    const OnSinksDiscoveredCallback& callback,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    std::unique_ptr<CastMediaSinkServiceImpl,
                    content::BrowserThread::DeleteOnIOThread>
        cast_media_sink_service_impl)
    : MediaSinkService(callback),
      task_runner_(task_runner),
      cast_media_sink_service_impl_(std::move(cast_media_sink_service_impl)) {}

CastMediaSinkService::~CastMediaSinkService() {}

void CastMediaSinkService::Start() {
  // TODO(crbug.com/749305): Migrate the discovery code to use sequences.
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (dns_sd_registry_)
    return;

  if (!cast_media_sink_service_impl_) {
    cast_media_sink_service_impl_.reset(new CastMediaSinkServiceImpl(
        base::BindRepeating(&CastMediaSinkService::OnSinksDiscoveredOnIOThread,
                            this),
        base::BindRepeating(
            &CastMediaSinkService::OnSinkAppAvailabilityUpdatedOnIOThread,
            this),
        cast_channel::CastSocketService::GetInstance(),
        DiscoveryNetworkMonitor::GetInstance(),
        Profile::FromBrowserContext(browser_context_)->GetRequestContext(),
        task_runner_));
  }

  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&CastMediaSinkServiceImpl::Start,
                                cast_media_sink_service_impl_->AsWeakPtr()));

  dns_sd_registry_ = DnsSdRegistry::GetInstance();
  dns_sd_registry_->AddObserver(this);
  dns_sd_registry_->RegisterDnsSdListener(kCastServiceType);
}

void CastMediaSinkService::Stop() {
  // TODO(crbug.com/749305): Migrate the discovery code to use sequences.
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!dns_sd_registry_)
    return;

  dns_sd_registry_->UnregisterDnsSdListener(kCastServiceType);
  dns_sd_registry_->RemoveObserver(this);
  dns_sd_registry_ = nullptr;

  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&CastMediaSinkServiceImpl::Stop,
                                cast_media_sink_service_impl_->AsWeakPtr()));

  cast_media_sink_service_impl_.reset();
}

void CastMediaSinkService::ForceSinkDiscoveryCallback() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!cast_media_sink_service_impl_)
    return;

  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&CastMediaSinkServiceImpl::ForceSinkDiscoveryCallback,
                     cast_media_sink_service_impl_->AsWeakPtr()));
}

void CastMediaSinkService::OnUserGesture() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (dns_sd_registry_)
    dns_sd_registry_->ForceDiscovery();

  if (!cast_media_sink_service_impl_)
    return;

  DVLOG(2) << "OnUserGesture: open channel now for " << cast_sinks_.size()
           << " devices discovered in latest round of mDNS";
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&CastMediaSinkServiceImpl::AttemptConnection,
                     cast_media_sink_service_impl_->AsWeakPtr(), cast_sinks_));
}

void CastMediaSinkService::RegisterMediaSinksObserver(
    MediaSinksObserver* observer) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(observer);

  const MediaSource& source = observer->source();
  std::unique_ptr<ParsedMediaSource> parsed = ParsedMediaSource::From(source);
  if (!parsed)
    return;

  // A broadcast request is not an actual sink query; it is used to send a
  // app precache message to receivers.
  if (parsed->broadcast_request) {
    // TODO(imcheng): Implement broadcast.
    return;
  }

  const MediaSource::Id& source_id = source.id();
  DCHECK(!base::ContainsKey(sinks_observers_, source_id));
  sinks_observers_.emplace(source_id,
                           MediaSinksObserverEntry(observer, *parsed));

  bool may_have_cached_results = false;
  std::vector<CastAppId> new_app_ids;
  for (const auto& app_info : parsed->app_infos) {
    const auto& app_id = app_info.app_id;
    if (++observer_count_by_app_id_[app_id] == 1)
      new_app_ids.push_back(app_id);

    // We may have results from previous queries.
    may_have_cached_results =
        may_have_cached_results ||
        base::ContainsKey(available_sinks_by_app_id_, app_id);
  }

  if (!new_app_ids.empty()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&CastMediaSinkServiceImpl::AddAppAvailabilityRequest,
                       cast_media_sink_service_impl_->AsWeakPtr(),
                       std::move(new_app_ids)));
  }

  if (may_have_cached_results) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(&CastMediaSinkService::NotifyOfExistingSinksIfRegistered,
                       this, source_id, observer));
  }
}

void CastMediaSinkService::NotifyOfExistingSinksIfRegistered(
    const MediaSource::Id& source_id,
    MediaSinksObserver* observer) {
  auto it = sinks_observers_.find(source_id);
  if (it == sinks_observers_.end())
    return;

  const auto& entry = it->second;
  entry.observer->OnSinksUpdated(
      GetSinksForSource(entry.parsed_source.app_infos),
      std::vector<url::Origin>());
}

void CastMediaSinkService::UnregisterMediaSinksObserver(
    MediaSinksObserver* observer) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(observer);

  const MediaSource& source = observer->source();
  auto observer_it = sinks_observers_.find(source.id());
  if (observer_it == sinks_observers_.end() ||
      observer_it->second.observer != observer)
    return;

  sinks_observers_.erase(observer_it);

  std::vector<CastAppId> app_ids_to_remove;
  auto it = observer_count_by_app_id_.begin();
  while (it != observer_count_by_app_id_.end()) {
    if (--(it->second) == 0) {
      app_ids_to_remove.push_back(it->first);
      observer_count_by_app_id_.erase(it++);
    } else {
      ++it;
    }
  }

  if (!app_ids_to_remove.empty()) {
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&CastMediaSinkServiceImpl::RemoveAppAvailabilityRequest,
                       cast_media_sink_service_impl_->AsWeakPtr(),
                       std::move(app_ids_to_remove)));
  }
}

void CastMediaSinkService::OnSinkAppAvailabilityUpdated(const MediaSink& sink,
                                                        CastAppId app_id,
                                                        bool available) {
  if (available) {
    AddSinkForApp(sink, app_id);
  } else {
    RemoveSinkForApp(sink, app_id);
  }

  // Notify observers of affected queries.
  for (const auto& source_and_entry : sinks_observers_) {
    const auto& entry = source_and_entry.second;
    if (entry.parsed_source.ContainsApp(app_id)) {
      // All origins are valid.
      // TODO(imcheng): Consider updating the MediaSinksObserver interface to
      // support partial updates (add/remove).
      entry.observer->OnSinksUpdated(
          GetSinksForSource(entry.parsed_source.app_infos),
          std::vector<url::Origin>());
    }
  }
}

std::vector<MediaSink> CastMediaSinkService::GetSinksForSource(
    const std::vector<CastAppInfo>& app_infos) {
  // Compare using sink IDs.
  base::flat_set<MediaSink, MediaSink::Compare> all_sinks;
  for (const auto& info : app_infos) {
    auto sinks_it = available_sinks_by_app_id_.find(info.app_id);
    if (sinks_it != available_sinks_by_app_id_.end()) {
      for (const auto& id_and_sink : sinks_it->second)
        all_sinks.insert(id_and_sink.second);
    }
  }
  return std::vector<MediaSink>(all_sinks.begin(), all_sinks.end());
}

void CastMediaSinkService::AddSinkForApp(const MediaSink& sink,
                                         CastAppId app_id) {
  available_sinks_by_app_id_[app_id].insert_or_assign(sink.id(), sink);
}

void CastMediaSinkService::RemoveSinkForApp(const MediaSink& sink,
                                            CastAppId app_id) {
  auto it = available_sinks_by_app_id_.find(app_id);
  if (it == available_sinks_by_app_id_.end())
    return;

  it->second.erase(sink.id());
  if (it->second.empty())
    available_sinks_by_app_id_.erase(it);
}

void CastMediaSinkService::SetDnsSdRegistryForTest(DnsSdRegistry* registry) {
  DCHECK(!dns_sd_registry_);
  dns_sd_registry_ = registry;
  dns_sd_registry_->AddObserver(this);
  dns_sd_registry_->RegisterDnsSdListener(kCastServiceType);
}

void CastMediaSinkService::OnDnsSdEvent(
    const std::string& service_type,
    const DnsSdRegistry::DnsSdServiceList& services) {
  // TODO(crbug.com/749305): Migrate the discovery code to use sequences.
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DVLOG(2) << "CastMediaSinkService::OnDnsSdEvent found " << services.size()
           << " services";

  cast_sinks_.clear();

  for (const auto& service : services) {
    // Create Cast sink from mDNS service description.
    MediaSinkInternal cast_sink;
    ErrorType error = CreateCastMediaSink(service, &cast_sink);
    if (error != ErrorType::NONE) {
      DVLOG(2) << "Fail to create Cast device [error]: " << error;
      continue;
    }

    cast_sinks_.push_back(cast_sink);
  }

  // Add a random backoff between 0s to 5s before opening channels to prevent
  // different browser instances connecting to the same receiver at the same
  // time.
  base::TimeDelta delay =
      base::TimeDelta::FromMilliseconds(base::RandInt(0, 50) * 100);
  DVLOG(2) << "Open channels in [" << delay.InSeconds() << "] seconds";

  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&CastMediaSinkServiceImpl::OpenChannels,
                     cast_media_sink_service_impl_->AsWeakPtr(), cast_sinks_,
                     CastMediaSinkServiceImpl::SinkSource::kMdns),
      delay);
}

void CastMediaSinkService::OnDialSinkAdded(const MediaSinkInternal& sink) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  cast_media_sink_service_impl_->OnDialSinkAdded(sink);
}

void CastMediaSinkService::OnSinksDiscoveredOnIOThread(
    std::vector<MediaSinkInternal> sinks) {
  // TODO(crbug.com/749305): Migrate the discovery code to use sequences.
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(sink_discovery_callback_, std::move(sinks)));
}

void CastMediaSinkService::OnSinkAppAvailabilityUpdatedOnIOThread(
    const MediaSink& sink,
    CastAppId app_id,
    bool available) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::BindOnce(&CastMediaSinkService::OnSinkAppAvailabilityUpdated, this,
                     sink, app_id, available));
}

CastMediaSinkService::MediaSinksObserverEntry::MediaSinksObserverEntry(
    MediaSinksObserver* observer,
    ParsedMediaSource parsed_source)
    : observer(observer), parsed_source(parsed_source) {}
CastMediaSinkService::MediaSinksObserverEntry::~MediaSinksObserverEntry() =
    default;

}  // namespace media_router
