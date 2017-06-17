// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_downloader.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "components/download/public/download_service.h"
#include "components/offline_pages/core/prefetch/prefetch_server_urls.h"
#include "url/gurl.h"

namespace offline_pages {

PrefetchDownloader::PrefetchDownloader(
    download::DownloadService* download_service,
    version_info::Channel channel)
    : download_service_(download_service),
      channel_(channel),
      service_started_(false) {
  DCHECK(download_service);
  service_started_ = download_service->GetStatus() ==
                     download::DownloadService::ServiceStatus::READY;
}

PrefetchDownloader::~PrefetchDownloader() = default;

void PrefetchDownloader::StartDownload(const std::string& download_id,
                                       const std::string& download_location) {
  if (!service_started_) {
    pending_downloads_.insert(std::make_pair(download_id, download_location));
    return;
  }

  download::DownloadParams params;
  params.client = download::DownloadClient::OFFLINE_PAGE_PREFETCH;
  params.guid = download_id;
  params.callback =
      base::Bind(&PrefetchDownloader::OnStartDownload, base::Unretained(this));
  params.request_params.url =
      GetPrefetchDownloadURL(download_location, channel_);
  download_service_->StartDownload(params);
}

void PrefetchDownloader::CancelDownload(const std::string& download_id) {
  if (service_started_)
    download_service_->CancelDownload(download_id);
  else
    pending_downloads_.erase(download_id);
}

void PrefetchDownloader::OnDownloadServiceReady() {
  service_started_ = true;
  for (const auto& entry : pending_downloads_)
    StartDownload(entry.first, entry.second);
  pending_downloads_.clear();
}

void PrefetchDownloader::OnStartDownload(
    const std::string& guid,
    download::DownloadParams::StartResult result) {
  // TODO(jianli): Notify prefetch service.
}

}  // namespace offline_pages
