// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_MOVE_PAGE_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_MOVE_PAGE_TASK_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/offline_page_item.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

class OfflinePageMetadataStoreSQL;

enum class MovePageTaskDisposition {
  MOVE_FAILED = 0,
  DB_WRITE_FAILED = 1,
  SUCCESS = 2
};

// Completion callback prototype for the MovePageTask.
using MovePageCallback = base::OnceCallback<void(MovePageTaskDisposition result,
                                                 const base::FilePath& path)>;

// This task exists to move an offline page from a private directory inside the
// chromium directory to a pubicly accessible directory such as
// /sdcard/Download.
class MovePageTask : public Task {
 public:
  MovePageTask(OfflinePageMetadataStoreSQL* store,
               int64_t offline_id,
               const base::FilePath& existing_file_path_,
               const base::FilePath& destination_file_path,
               MovePageCallback callback);
  ~MovePageTask() override;

  // Task implementation
  void Run() override;

 private:
  // Internal callback
  void OnMovePageDone(MovePageTaskDisposition result);

  // Unowned pointer to the Metadata SQL store.
  OfflinePageMetadataStoreSQL* store_;

  // ID of the offline page we are moving.
  const int64_t offline_id_;

  // Current location and destination for the offline page.
  const base::FilePath existing_file_path_;
  const base::FilePath destination_file_path_;

  MovePageCallback callback_;

  base::WeakPtrFactory<MovePageTask> weak_ptr_factory_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_MOVE_PAGE_TASK_H_
