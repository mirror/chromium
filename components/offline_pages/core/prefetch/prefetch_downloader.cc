// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/prefetch/prefetch_downloader.h"

#include "base/bind.h"
#include "base/logging.h"
#include "components/download/public/download_service.h"
#include "components/offline_pages/core/prefetch/prefetch_server_urls.h"
#include "url/gurl.h"

namespace offline_pages {

PrefetchDownloader::PrefetchDownloader(
    download::DownloadService* download_service,
    version_info::Channel channel)
    : download_service_(download_service), channel_(channel) {
  DCHECK(download_service);
}

PrefetchDownloader::~PrefetchDownloader() {}

void PrefetchDownloader::StartDownload(const std::string& download_id,
                                       const std::string& download_location) {
  download::DownloadParams params;
  params.client = download::DownloadClient::OFFLINE_PAGE_PREFETCH;
  params.guid = download_id;
  params.callback =
      base::Bind(&PrefetchDownloader::OnStartDownload, base::Unretained(this));
  params.request_params.url =
      GetPrefetchDownloadURL(download_location, channel_);
  download_service_->StartDownload(params);
}

void PrefetchDownloader::OnStartDownload(
    const std::string& guid,
    download::DownloadParams::StartResult result) {}

}  // offline_pages
