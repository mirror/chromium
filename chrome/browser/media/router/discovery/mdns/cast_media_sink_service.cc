// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service.h"

#include "base/memory/ptr_util.h"
#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service_impl.h"
#include "chrome/common/media_router/discovery/media_sink_internal.h"
#include "components/cast_channel/cast_socket_service.h"
#include "net/base/host_port_pair.h"

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
                               media_router::MediaSink::IconType::CAST);

  auto host_port_pair =
      net::HostPortPair::FromString(service.service_host_port);

  media_router::CastSinkExtraData extra_data;
  extra_data.ip_address = ip_address;
  extra_data.port = host_port_pair.port();
  extra_data.model_name = service_data["md"];

  cast_sink->set_sink(sink);
  cast_sink->set_cast_data(extra_data);

  return ErrorType::NONE;
}

}  // namespace

namespace media_router {

// static
const char CastMediaSinkService::kCastServiceType[] = "_googlecast._tcp.local";

CastMediaSinkService::CastMediaSinkService(
    const OnSinksDiscoveredCallback& callback,
    content::BrowserContext* browser_context)
    : MediaSinkService(callback),
      cast_socket_service_(cast_channel::CastSocketService::GetInstance()) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DCHECK(cast_socket_service_);
}

CastMediaSinkService::~CastMediaSinkService() {}

void CastMediaSinkService::Start() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (dns_sd_registry_)
    return;

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&CastMediaSinkService::StartOnIOThread, this));

  dns_sd_registry_ = DnsSdRegistry::GetInstance();
  dns_sd_registry_->AddObserver(this);
  dns_sd_registry_->RegisterDnsSdListener(kCastServiceType);
}

void CastMediaSinkService::Stop() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (!dns_sd_registry_)
    return;

  dns_sd_registry_->UnregisterDnsSdListener(kCastServiceType);
  dns_sd_registry_->RemoveObserver(this);
  dns_sd_registry_ = nullptr;

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&CastMediaSinkService::StopOnIOThread, this));
}

void CastMediaSinkService::SetDnsSdRegistryForTest(DnsSdRegistry* registry) {
  DCHECK(!dns_sd_registry_);
  dns_sd_registry_ = registry;
  dns_sd_registry_->AddObserver(this);
  dns_sd_registry_->RegisterDnsSdListener(kCastServiceType);
}

void CastMediaSinkService::SetCastMediaSinkServiceImplForTest(
    std::unique_ptr<CastMediaSinkServiceImpl> cast_media_sink_service_impl) {
  DCHECK(!cast_media_sink_service_impl_);
  cast_media_sink_service_impl_ = std::move(cast_media_sink_service_impl);
}

void CastMediaSinkService::OnDnsSdEvent(
    const std::string& service_type,
    const DnsSdRegistry::DnsSdServiceList& services) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DVLOG(2) << "CastMediaSinkService::OnDnsSdEvent found " << services.size()
           << " services";

  std::vector<MediaSinkInternal> cast_sinks;

  for (const auto& service : services) {
    // Create Cast sink from mDNS service description.
    MediaSinkInternal cast_sink;
    ErrorType error = CreateCastMediaSink(service, &cast_sink);
    if (error != ErrorType::NONE) {
      DVLOG(2) << "Fail to create Cast device [error]: " << error;
      continue;
    }

    cast_sinks.push_back(std::move(cast_sink));
  }

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&CastMediaSinkService::OpenChannelsOnIOThread, this,
                 std::move(cast_sinks)));
}

void CastMediaSinkService::StartOnIOThread() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!cast_media_sink_service_impl_) {
    cast_media_sink_service_impl_ = base::MakeUnique<CastMediaSinkServiceImpl>(
        base::Bind(&CastMediaSinkService::OnSinksDiscoveredOnIOThread, this),
        cast_socket_service_);
  }

  cast_media_sink_service_impl_->Start();
}

void CastMediaSinkService::StopOnIOThread() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!cast_media_sink_service_impl_)
    return;

  cast_media_sink_service_impl_->Stop();
  cast_media_sink_service_impl_.reset();
}

void CastMediaSinkService::OnSinksDiscoveredOnIOThread(
    std::vector<MediaSinkInternal> sinks) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  content::BrowserThread::PostTask(
      content::BrowserThread::UI, FROM_HERE,
      base::Bind(sink_discovery_callback_, std::move(sinks)));
}

void CastMediaSinkService::OpenChannelsOnIOThread(
    std::vector<MediaSinkInternal> cast_sinks) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (cast_media_sink_service_impl_)
    cast_media_sink_service_impl_->OpenChannels(std::move(cast_sinks));
}

}  // namespace media_router
