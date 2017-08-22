// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_SAVE_PAGE_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_SAVE_PAGE_TASK_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "components/offline_pages/core/offline_page_archiver.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "components/offline_pages/core/offline_page_types.h"
#include "components/offline_pages/core/offline_store_types.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

class ArchiveManager;
class OfflinePageMetadataStore;

class SavePageTask : public Task {
 public:
  typedef base::Callback<void(SavePageResult, const OfflinePageItem&)>
      SavePageTaskCallback;

  SavePageTask(OfflinePageModel* model,
               const OfflinePageModel::SavePageParams& save_page_params,
               std::unique_ptr<OfflinePageArchiver> archiver,
               const SavePageTaskCallback& callback);
  ~SavePageTask() override;

  // Task implementation.
  void Run() override;

  // Methods for testing only.
  void set_testing_clock(base::Clock* clock) { testing_clock_ = clock; }

 private:
  void SavePage();
  void OnCreateArchiveDone(const base::Time& start_time,
                           OfflinePageArchiver* archiver,
                           OfflinePageArchiver::ArchiverResult archive_result,
                           const GURL& saved_url,
                           const base::FilePath& file_path,
                           const base::string16& title,
                           int64_t file_size);
  void OnAddOfflinePageDone(AddPageResult add_result, int64_t offline_id);
  void InformSavePageDone(SavePageResult result);
  void DeletePendingArchiver(OfflinePageArchiver* archiver);

  base::Time GetCurrentTime() const;

  OfflinePageModel* model_;
  OfflinePageModel::SavePageParams save_page_params_;
  SavePageTaskCallback callback_;
  std::unique_ptr<OfflinePageArchiver> archiver_;
  int64_t offline_id_;
  OfflinePageItem offline_page_;

  base::Clock* testing_clock_;

  base::WeakPtrFactory<SavePageTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(SavePageTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_SAVE_PAGE_TASK_H_
