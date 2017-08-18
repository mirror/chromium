// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_DOWNLOAD_CLEANUP_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_DOWNLOAD_CLEANUP_TASK_H_

#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {
class PrefetchStore;

// Reconciliation task for cleaning up database entries that are in DOWNLOADING
// state. This is indeed triggered only when the download service is ready and
// notifies us about the ongoing and completed downloads.
class DownloadCleanupTask : public Task {
 public:
  // Maximum number of attempts to retry a download.
  static const int kMaxDownloadAttempts;

  DownloadCleanupTask(
      PrefetchStore* prefetch_store,
      const std::vector<std::string>& outstanding_download_ids,
      const std::vector<PrefetchDownloadResult>& success_downloads);
  ~DownloadCleanupTask() override;

  void Run() override;

 private:
  void OnFinished(bool success);

  PrefetchStore* prefetch_store_;  // Outlives this class.
  std::vector<std::string> outstanding_download_ids_;
  std::vector<PrefetchDownloadResult> success_downloads_;

  base::WeakPtrFactory<DownloadCleanupTask> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DownloadCleanupTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_DOWNLOAD_CLEANUP_TASK_H_
