// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_CLEAR_STORAGE_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_CLEAR_STORAGE_TASK_H_

#include <utility>

#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/archive_manager.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/task.h"

namespace base {
class Clock;
class Time;
}  // namespace base

namespace offline_pages {

class ArchiveManager;
class ClientPolicyController;
class OfflinePageMetadataStoreSQL;

class ClearStorageTask : public Task {
 public:
  enum class ClearStorageResult {
    SUCCESS,                                // Cleared successfully.
    UNNECESSARY,                            // No expired pages.
    DEPRECATED_EXPIRE_FAILURE,              // Expiration failed. (DEPRECATED)
    DELETE_FAILURE,                         // Deletion failed.
    DEPRECATED_EXPIRE_AND_DELETE_FAILURES,  // Both expiration and deletion
                                            // failed. (DEPRECATED)
    // NOTE: always keep this entry at the end. Add new result types only
    // immediately above this line. Make sure to update the corresponding
    // histogram enum accordingly.
    RESULT_COUNT,
  };

  // Enum indicating how to clear the pages.
  enum class ClearMode {
    // Using normal expiration logic to clear pages. Will reduce the storage
    // usage down below the threshold.
    DEFAULT,
    // No need to clear any page (no pages in the model or no expired pages and
    // we're not exceeding the storage limit.)
    NOT_NEEDED,
  };

  // Callback used when calling ClearPagesIfNeeded.
  // size_t: the number of cleared pages.
  // ClearStorageResult: result of clearing pages in storage.
  typedef base::Callback<void(const base::Time&, size_t, ClearStorageResult)>
      ClearStorageCallback;

  ClearStorageTask(OfflinePageMetadataStoreSQL* store,
                   ArchiveManager* archive_manager,
                   ClientPolicyController* policy_controller,
                   const base::Time& last_start_time,
                   const ClearStorageCallback& callback);
  ~ClearStorageTask() override;

  // Task implementation.
  void Run() override;

  // Testing only.
  void SetClockForTesting(std::unique_ptr<base::Clock> clock);

 private:
  void OnGetStorageStatsDone(const ArchiveManager::StorageStats& stats);
  void OnGetAllPagesDone(std::unique_ptr<MultipleOfflinePageItemResult> pages);
  void OnClearPagesDone(std::pair<size_t, DeletePageResult> result);
  void InformClearStorageDone(size_t pages_cleared, ClearStorageResult result);

  OfflinePageMetadataStoreSQL* store_;
  ArchiveManager* archive_manager_;
  ClientPolicyController* policy_controller_;
  ClearStorageCallback callback_;
  base::Time last_start_time_;
  base::Time start_time_;

  // Clock for getting time.
  std::unique_ptr<base::Clock> clock_;

  base::WeakPtrFactory<ClearStorageTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(ClearStorageTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_CLEAR_STORAGE_TASK_H_
