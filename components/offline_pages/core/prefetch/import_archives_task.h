// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_IMPORT_ARCHIVES_TASK_H_
#define COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_IMPORT_ARCHIVES_TASK_H_

#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/prefetch/prefetch_importer.h"
#include "components/offline_pages/core/prefetch/prefetch_types.h"
#include "components/offline_pages/core/task.h"

namespace offline_pages {

class PrefetchStore;

// Task that attempts to import downloaded archives to offline page model.
class ImportArchivesTask : public Task {
 public:
  ImportArchivesTask(PrefetchStore* prefetch_store,
                     PrefetchImporter* prefetch_importer);
  ~ImportArchivesTask() override;

  // Task implementation.
  void Run() override;

 private:
  void Process();
  void OnArchivesRetrieved(std::vector<PrefetchImporter::ArchiveInfo> archives);
  void OnStateUpdatedToImporting(const PrefetchImporter::ArchiveInfo& archive,
                                 bool success);
  void OnArchiveImported(bool success, int64_t offline_id);
  void OnStateUpdatedToFinished(bool success);

  PrefetchStore* prefetch_store_;        // Outlives this class.
  PrefetchImporter* prefetch_importer_;  // Outlives this class.
  std::vector<PrefetchImporter::ArchiveInfo> archives_;

  base::WeakPtrFactory<ImportArchivesTask> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ImportArchivesTask);
};

}  // namespace offline_pages

#endif  // COMPONENTS_OFFLINE_PAGES_CORE_PREFETCH_IMPORT_ARCHIVES_TASK_H_
