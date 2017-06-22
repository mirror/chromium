// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_TEST_PREFETCH_DISPATCHER_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_TEST_PREFETCH_DISPATCHER_H_

#include <memory>
#include <string>
#include <vector>

#include "components/offline_pages/core/prefetch/prefetch_dispatcher.h"

namespace offline_pages {
struct ClientId;

// Testing version of the prefetch dispatcher.
class TestPrefetchDispatcher : public PrefetchDispatcher {
 public:
  struct DownloadResult {
    DownloadResult();
    ~DownloadResult();

    std::string download_id;
    bool success = false;
    base::FilePath file_path;
    uint64_t file_size = 0u;
  };

  TestPrefetchDispatcher();
  ~TestPrefetchDispatcher() override;

  // PrefetchDispatcher implementation.
  void AddCandidatePrefetchURLs(
      const std::string& name_space,
      const std::vector<PrefetchURL>& prefetch_urls) override;
  void RemoveAllUnprocessedPrefetchURLs(const std::string& name_space) override;
  void RemovePrefetchURLsByClientId(const ClientId& client_id) override;
  void BeginBackgroundTask(std::unique_ptr<ScopedBackgroundTask> task) override;
  void StopBackgroundTask() override;
  void SetService(PrefetchService* service) override;
  void GCMOperationCompletedMessageReceived(
      const std::string& operation_name) override;
  void DownloadSucceeded(const std::string& download_id,
                         const base::FilePath& file_path,
                         uint64_t file_size) override;
  void DownloadFailed(const std::string& download_id) override;
  void RequestFinishBackgroundTaskForTest() override;

  std::string latest_name_space;
  std::vector<PrefetchURL> latest_prefetch_urls;
  std::unique_ptr<ClientId> last_removed_client_id;
  std::vector<std::string> operation_list;
  std::vector<DownloadResult> completed_downloads_;

  int new_suggestions_count = 0;
  int remove_all_suggestions_count = 0;
  int remove_by_client_id_count = 0;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_TEST_PREFETCH_DISPATCHER_H_
