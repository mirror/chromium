// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DOWNLOAD_INTERNAL_FILE_MONITOR_H_
#define COMPONENTS_DOWNLOAD_INTERNAL_FILE_MONITOR_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "components/download/internal/driver_entry.h"
#include "components/download/internal/model.h"
#include "components/download/internal/stats.h"

namespace download {

struct Entry;

// An utility class containing various file cleanup methods.
class FileMonitor {
 public:
  FileMonitor(
      const base::FilePath& download_file_dir,
      const scoped_refptr<base::SequencedTaskRunner>& file_thread_task_runner,
      base::TimeDelta file_keep_alive_time);
  virtual ~FileMonitor();

  // Deletes the files in storage directory that are not related to any entries
  // in either database.
  virtual void DeleteUnknownFiles(
      const Model::EntryList& known_entries,
      const std::vector<DriverEntry>& known_driver_entries);

  // Deletes the files for the database entries which have been completed and
  // ready for cleanup. Returns the entries eligible for clean up.
  virtual std::vector<Entry*> CleanupFilesForCompletedEntries(
      const Model::EntryList& entries);

  // Deletes a list of files and logs UMA.
  void DeleteFiles(const std::vector<base::FilePath>& files_to_remove,
                   stats::FileCleanupReason reason);

 private:
  void DeleteUnknownFilesOnFileThread(
      const std::set<base::FilePath>& paths_in_db);
  void DeleteFilesOnFileThread(const std::vector<base::FilePath>& paths,
                               stats::FileCleanupReason reason);
  bool ReadyForCleanup(const Entry* entry);

  const base::FilePath download_file_dir_;
  const base::TimeDelta file_keep_alive_time_;

  scoped_refptr<base::SequencedTaskRunner> file_thread_task_runner_;
  base::WeakPtrFactory<FileMonitor> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FileMonitor);
};

}  // namespace download

#endif  // COMPONENTS_DOWNLOAD_INTERNAL_FILE_MONITOR_H_
