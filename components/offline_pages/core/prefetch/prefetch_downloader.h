// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_DOWNLOADER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_DOWNLOADER_H_

#include <map>

#include "base/callback.h"
#include "base/macros.h"
#include "components/download/public/download_params.h"
#include "components/version_info/channel.h"

namespace download {
class DownloadService;
}

namespace offline_pages {

// Asynchronously downloads the archive.
class PrefetchDownloader {
 public:
  PrefetchDownloader(download::DownloadService* download_service,
                     version_info::Channel channel);
  ~PrefetchDownloader();

  void StartDownload(const std::string& download_id,
                     const std::string& download_location);
  void CancelDownload(const std::string& download_id);

  // Called to inform that the download service is ready.
  void OnDownloadServiceReady();

 private:
  void OnStartDownload(const std::string& guid,
                       download::DownloadParams::StartResult result);

  download::DownloadService* download_service_;
  version_info::Channel channel_;
  bool service_started_;
  std::map<std::string, std::string> pending_downloads_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchDownloader);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_DOWNLOADER_H_
