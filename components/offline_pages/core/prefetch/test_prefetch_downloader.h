// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_TEST_PREFETCH_DOWNLOADER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_TEST_PREFETCH_DOWNLOADER_H_

#include <map>
#include <string>

#include "components/offline_pages/core/prefetch/prefetch_downloader.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"

namespace offline_pages {

// Asynchronously downloads the archive.
class TestPrefetchDownloader : public PrefetchDownloader {
 public:
  TestPrefetchDownloader();
  ~TestPrefetchDownloader() override;

  void SetCompletedCallback(
      const PrefetchDownloadCompletedCallback& callback) override;
  void StartDownload(const std::string& download_id,
                     const std::string& download_location) override;
  void CancelDownload(const std::string& download_id) override;
  void OnDownloadServiceReady() override;
  void OnDownloadServiceShutdown() override;
  void OnDownloadSucceeded(const std::string& download_id,
                           const base::FilePath& file_path,
                           uint64_t file_size) override;
  void OnDownloadFailed(const std::string& download_id) override;

  void Reset();

  const std::map<std::string, std::string>& requested_downloads() const {
    return requested_downloads_;
  }

 private:
  std::map<std::string, std::string> requested_downloads_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_TEST_PREFETCH_DOWNLOADER_H_
