// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_ADD_PAGE_TO_DOWNLOAD_MANAGER_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_ADD_PAGE_TO_DOWNLOAD_MANAGER_TASK_H_

#include <memory>
#include <string>
#include <vector>

#include "stdint.h"

#include "components/offline_pages/core/offline_page_metadata_store_sql.h"
#include "components/offline_pages/core/system_download_manager.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

class AddPageToDownloadManagerTask : public Task {
 public:
  AddPageToDownloadManagerTask(OfflinePageMetadataStoreSQL* store,
                               SystemDownloadManager* download_manager,
                               int64_t offline_id,
                               const std::string& title,
                               const std::string& description,
                               const std::string& path,
                               long length,
                               const std::string& uri,
                               const std::string& referer);
  ~AddPageToDownloadManagerTask() override;

  // Task implementation
  void Run() override;

  // Internal callback
  void OnAddIdDone(bool result);

 private:
  OfflinePageMetadataStoreSQL* store_;
  const std::string title_;
  const std::string description_;
  const std::string path_;
  const std::string uri_;
  const std::string referer_;
  int64_t offline_id_;
  long length_;
  // unowned pointer to a download manager for this system, if any.
  SystemDownloadManager* download_manager_;

  base::WeakPtrFactory<AddPageToDownloadManagerTask> weak_ptr_factory_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_ADD_PAGE_TO_DOWNLOAD_MANAGER_TASK_H_
