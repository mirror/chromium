// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_FILE_MONITOR_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_FILE_MONITOR_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/sequenced_task_runner.h"
#include "components/download/internal/client_set.h"
#include "components/download/internal/download_driver.h"
#include "components/download/internal/model.h"
#include "components/download/internal/stats.h"

namespace download {

struct Entry;

// A callback used to remove a list of entries from the model.
using RemoveEntriesCallback = base::Callback<void(const std::vector<Entry*>&)>;

// An utility class containing various cleanup methods.
class FileMonitor {
 public:
  FileMonitor(
      const base::FilePath& download_dir,
      const scoped_refptr<base::SequencedTaskRunner>& file_thread_task_runner,
      base::TimeDelta file_keep_alive_time);
  ~FileMonitor();

  // Removes the files in storage directory that are not related to any entries
  // in the database.
  void RemoveUnassociatedFiles(const Model::EntryList& entries,
                               const std::vector<DriverEntry>& driver_entries);

  // Removes the entries from the database which have been completed and ready
  // for cleanup.
  void RemoveEntriesAfterTimeout(const Model::EntryList& entries,
                                 const RemoveEntriesCallback& callback);

 private:
  void RemoveUnassociatedFilesOnFileThread(
      const std::set<base::FilePath>& paths_in_db);
  void DeleteFilesOnFileThread(
      const std::vector<base::FilePath>& paths,
      const base::Optional<stats::FileCleanupReason>& reason);
  bool ReadyForCleanup(const Entry* entry);

  base::FilePath download_dir_;
  scoped_refptr<base::SequencedTaskRunner> file_thread_task_runner_;
  base::TimeDelta file_keep_alive_time_;
  base::WeakPtrFactory<FileMonitor> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FileMonitor);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_FILE_MONITOR_H_
