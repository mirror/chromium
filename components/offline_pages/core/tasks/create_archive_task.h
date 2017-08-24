// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_TASKS_CREATE_ARCHIVE_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_TASKS_CREATE_ARCHIVE_TASK_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "components/offline_pages/core/offline_page_archiver.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/offline_store_types.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

class CreateArchiveTask : public Task {
 public:
  typedef base::Callback<void(OfflinePageArchiver::ArchiverResult,
                              OfflinePageItem)>
      CreateArchiveTaskCallback;

  CreateArchiveTask(const base::FilePath& archives_dir,
                    const OfflinePageModel::SavePageParams& save_page_params,
                    std::unique_ptr<OfflinePageArchiver> archiver,
                    const CreateArchiveTaskCallback& callback);
  ~CreateArchiveTask() override;

  // Task implementation.
  void Run() override;

  // Methods for testing only.
  void set_testing_clock(base::Clock* clock) { testing_clock_ = clock; }
  void set_skip_clearing_original_url_for_testing() {
    skip_clearing_original_url_for_testing_ = true;
  }

 private:
  void CreateArchive();
  void OnCreateArchiveDone(const base::Time& start_time,
                           OfflinePageArchiver* archiver,
                           OfflinePageArchiver::ArchiverResult archiver_result,
                           const GURL& saved_url,
                           const base::FilePath& file_path,
                           const base::string16& title,
                           int64_t file_size);
  void InformCreationFailed(OfflinePageArchiver::ArchiverResult result);
  void InformCreationDone(OfflinePageArchiver::ArchiverResult,
                          const OfflinePageItem& offline_page);
  void DeletePendingArchiver(OfflinePageArchiver* archiver);

  base::Time GetCurrentTime() const;

  base::FilePath archives_dir_;
  OfflinePageModel::SavePageParams save_page_params_;
  int64_t offline_id_;
  std::unique_ptr<OfflinePageArchiver> archiver_;
  CreateArchiveTaskCallback callback_;

  base::Clock* testing_clock_;
  bool skip_clearing_original_url_for_testing_;

  base::WeakPtrFactory<CreateArchiveTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(CreateArchiveTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_SAVE_PAGE_TASK_H_
