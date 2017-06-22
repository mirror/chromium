// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_DOWNLOADER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_DOWNLOADER_H_

#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "components/download/public/download_params.h"
#include "components/offline_pages/core/prefetch/prefetch_service.h"
#include "components/version_info/channel.h"

namespace download {
class DownloadService;
}  // namespace download

namespace offline_pages {

class PrefetchService;

// Asynchronously downloads the archive.
class PrefetchDownloader {
 public:
  explicit PrefetchDownloader(version_info::Channel channel);
  ~PrefetchDownloader();

  void SetPrefetchService(PrefetchService* service);

  // DownloadService should be ready at this time.
  void SetDownloadService(download::DownloadService* download_service);

  void StartDownload(const std::string& download_id,
                     const std::string& download_location);
  void CancelDownload(const std::string& download_id);

  // Responding to download client event.
  void OnDownloadSucceeded(const std::string& download_id,
                           const base::FilePath& file_path,
                           uint64_t file_size);
  void OnDownloadFailed(const std::string& download_id);

 private:
  // Callback for StartDownload.
  void OnStartDownload(const std::string& download_id,
                       download::DownloadParams::StartResult result);

  version_info::Channel channel_;

  // Unowned, owns |this|.
  PrefetchService* prefetch_service_ = nullptr;

  download::DownloadService* download_service_ = nullptr;

  // TODO(jianli): Investigate making PrefetchService waits for DownloadService
  // ready in order to avoid queueing.
  // List of downloads pending to start after the download service starts. Each
  // item is a pair of download id and download location.
  std::vector<std::pair<std::string, std::string>> pending_downloads_;
  // List of ids of downloads waiting to be cancelled after the download service
  // starts.
  std::vector<std::string> pending_cancellations_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchDownloader);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_PREFETCH_DOWNLOADER_H_
