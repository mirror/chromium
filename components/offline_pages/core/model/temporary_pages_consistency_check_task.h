// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_MODEL_TEMPORARY_PAGES_CONSISTENCY_CHECK_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_MODEL_TEMPORARY_PAGES_CONSISTENCY_CHECK_TASK_H_

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

class ClientPolicyController;
class OfflinePageMetadataStoreSQL;

// Task that does consistency check between temporary pages stored in metadata
// store and temporary archive files. No callback is required for this task.
// The task includes two steps:
// 1. Iterate through all temporary entries in the DB, delete the ones that
// do not have associated files.
// 2. Iterate through all files in the temporary archives directory, delete the
// ones that do not have associated DB entries.
// TODO(romax): https://crbug.com/772171. This also needs to be revised during
// P2P sharing implementation.
class TemporaryPagesConsistencyCheckTask : public Task {
 public:
  TemporaryPagesConsistencyCheckTask(OfflinePageMetadataStoreSQL* store,
                                     ClientPolicyController* policy_controller,
                                     const base::FilePath& archives_dir);
  ~TemporaryPagesConsistencyCheckTask() override;

  // Task implementation
  void Run() override;

 private:
  void OnCheckConsistencyDone(bool result);

  // The store for consistency check. Not owned.
  OfflinePageMetadataStoreSQL* store_;
  // The client policy controller to get policies for temporary namespaces.
  // Not owned.
  ClientPolicyController* policy_controller_;
  base::FilePath archives_dir_;

  base::WeakPtrFactory<TemporaryPagesConsistencyCheckTask> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(TemporaryPagesConsistencyCheckTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_MODEL_TEMPORARY_PAGES_CONSISTENCY_CHECK_TASK_H_
