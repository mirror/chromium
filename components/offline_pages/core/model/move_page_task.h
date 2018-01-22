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

typedef base::OnceCallback<void(const OfflinePageItem& page,
                                const base::FilePath& path)>
    MovePageCallback;

// This task exists to move an offline page from a private directory inside the
// chromium directory to a pubicly accessible directory such as
// /sdcard/Download.
class MovePageTask : public Task {
 public:
  MovePageTask(OfflinePageMetadataStoreSQL* store,
               const OfflinePageItem& page,
               const base::FilePath& public_directory,
               MovePageCallback callback);
  ~MovePageTask() override;

  // Task implementation
  void Run() override;

 private:
  // Internal callback
  void OnMovePageDone(bool result);

  // Unowned pointer to the Metadata SQL store.
  OfflinePageMetadataStoreSQL* store_;

  // Copy of the offline page item we were passed in the ctor to make sure it
  // stays in scope.
  const OfflinePageItem page_;

  // Destination for the offline page.
  const base::FilePath& public_directory_;
  // Fully qualified file path of the new page.
  base::FilePath new_file_path_;

  MovePageCallback callback_;

  base::WeakPtrFactory<MovePageTask> weak_ptr_factory_;
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_BACKGROUND_MOVE_PAGE_TASK_H_
