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

// Asynchronously downloads the archive.
class PrefetchDownloader : public PrefetchService::DownloadDelegate {
 public:
  using DownloadFinishedCallback =
      base::Callback<void(const std::string& download_id, bool success)>;

  PrefetchDownloader(download::DownloadService* download_service,
                     version_info::Channel channel,
                     const DownloadFinishedCallback& callback);
  ~PrefetchDownloader() override;

  void StartDownload(const std::string& download_id,
                     const std::string& download_location);
  void CancelDownload(const std::string& download_id);

  // DownloadDelegate implementations:
  void OnDownloadServiceReady() override;
  void OnDownloadSucceeded(const std::string& download_id,
                           const base::FilePath& path,
                           uint64_t size) override;
  void OnDownloadFailed(const std::string& download_id) override;

 private:
  // Callback for StartDownload.
  void OnStartDownload(const std::string& guid,
                       download::DownloadParams::StartResult result);

  download::DownloadService* download_service_;
  version_info::Channel channel_;
  DownloadFinishedCallback callback_;
  bool service_started_;
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
