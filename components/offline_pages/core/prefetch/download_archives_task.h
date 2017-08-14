// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_DOWNLOAD_ARCHIVES_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_DOWNLOAD_ARCHIVES_TASK_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {
class PrefetchDownloader;
class PrefetchStore;

class DownloadArchivesTask : public Task {
 public:
  using Params = std::vector<std::pair<std::string, std::string>>;

  DownloadArchivesTask(PrefetchStore* prefetch_store,
                       PrefetchDownloader* prefetch_downloader);
  ~DownloadArchivesTask() override;

  void Run() override;

 private:
  void SendItemsToPrefetchDownloader(std::unique_ptr<Params> download_params);

  // Prefetch store to execute against. Not owned.
  PrefetchStore* prefetch_store_;
  // Prefetch downloader to request downloads from. Not owned.
  PrefetchDownloader* prefetch_downloader_;

  base::WeakPtrFactory<DownloadArchivesTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(DownloadArchivesTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_DOWNLOAD_ARCHIVES_TASK_H_
