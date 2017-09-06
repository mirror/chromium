// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_TASKS_DELETE_PAGE_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_TASKS_DELETE_PAGE_TASK_H_

#include <memory>

#include "base/bind.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/offline_page_metadata_store.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/task.h"

namespace sql {
class Connection;
}  // namespace sql

namespace offline_pages {

struct ClientId;
class ArchiveManager;
class OfflinePageMetadataStoreSQL;

class DeletePageTask : public Task {
 public:
  typedef base::Callback<void(const std::vector<OfflinePageItem>&,
                              DeletePageResult)>
      DeletePageTaskCallback;
  typedef base::OnceCallback<std::unique_ptr<OfflinePagesUpdateResult>(
      sql::Connection*)>
      DeleteFunction;

  DeletePageTask(OfflinePageMetadataStoreSQL* store,
                 ArchiveManager* archive_manager,
                 DeleteFunction func,
                 const DeletePageTaskCallback& callback);
  ~DeletePageTask() override;

  // Task implementation.
  void Run() override;

 private:
  void OnDeletePageDone(std::unique_ptr<OfflinePagesUpdateResult> result);
  void OnDeleteArchiveFilesDone(
      std::unique_ptr<OfflinePagesUpdateResult> result,
      bool delete_files_result);
  void InformDeletePageDone(DeletePageResult result);

  OfflinePageMetadataStoreSQL* store_;
  ArchiveManager* archive_manager_;
  DeleteFunction func_;
  DeletePageTaskCallback callback_;

  base::WeakPtrFactory<DeletePageTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(DeletePageTask);
};

class DeletePageTaskFactory {
 public:
  static std::unique_ptr<DeletePageTask> CreateTaskWithOfflineIds(
      OfflinePageMetadataStoreSQL* store,
      ArchiveManager* archive_manager,
      const std::vector<int64_t>& offline_ids,
      const DeletePageTask::DeletePageTaskCallback& callback);
  static std::unique_ptr<DeletePageTask> CreateTaskWithClientIds(
      OfflinePageMetadataStoreSQL* store,
      ArchiveManager* archive_manager,
      const std::vector<ClientId>& client_ids,
      const DeletePageTask::DeletePageTaskCallback& callback);
  static std::unique_ptr<DeletePageTask> CreateTaskWithPagePredicate(
      OfflinePageMetadataStoreSQL* store,
      ArchiveManager* archive_manager,
      const PagePredicate& predicate,
      const DeletePageTask::DeletePageTaskCallback& callback);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_TASKS_DELETE_PAGE_TASK_H_
