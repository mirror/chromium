// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/internal/file_monitor.h"

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "components/download/internal/entry.h"

namespace download {

FileMonitor::FileMonitor(
    Model* model,
    DownloadDriver* driver,
    const base::FilePath& storage_dir,
    scoped_refptr<base::TaskRunner>& file_thread_task_runner,
    base::TimeDelta file_keep_alive_time)
    : model_(model),
      driver_(driver),
      storage_dir_(storage_dir),
      file_thread_task_runner_(file_thread_task_runner),
      file_keep_alive_time_(file_keep_alive_time),
      weak_factory_(this) {}

FileMonitor::~FileMonitor() = default;

void FileMonitor::RemoveUnassociatedFiles() {
  std::vector<std::string> guids;
  std::set<base::FilePath> download_file_paths;
  for (Entry* entry : model_->PeekEntries()) {
    download_file_paths.insert(entry->file_path);
    guids.push_back(entry->guid);
  }

  driver_->GetPhysicalFilePathForDownloads(download_file_paths, guids);
  file_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&FileMonitor::RemoveUnassociatedFilesOnFileThread,
                            weak_factory_.GetWeakPtr(), download_file_paths));
}

void FileMonitor::RemoveUnassociatedFilesOnFileThread(
    const std::set<base::FilePath>& download_file_paths) {
  std::set<base::FilePath> files_in_dir;
  base::FileEnumerator file_enumerator(storage_dir_, false, /* recursive */
                                       base::FileEnumerator::FILES);

  for (base::FilePath path = file_enumerator.Next(); !path.value().empty();
       path = file_enumerator.Next()) {
    files_in_dir.insert(path);
  }

  std::vector<base::FilePath> files_to_remove =
      base::STLSetDifference<std::vector<base::FilePath>>(files_in_dir,
                                                          download_file_paths);
  DeleteFiles(files_to_remove, stats::FileCleanupReason::ORPHANED);
}

void FileMonitor::RemoveEntriesAfterTimeout() {
  std::vector<base::FilePath> files_to_remove;
  for (Entry* entry : model_->PeekEntries()) {
    if (!ReadyForCleanup(entry))
      continue;

    model_->Remove(entry->guid);
    files_to_remove.push_back(entry->file_path);
  }

  file_thread_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&FileMonitor::DeleteFiles, weak_factory_.GetWeakPtr(),
                 files_to_remove, stats::FileCleanupReason::TIMEOUT));
}

void FileMonitor::DeleteFiles(
    const std::vector<base::FilePath>& paths,
    const base::Optional<stats::FileCleanupReason>& reason) {
  int num_delete_attempted = 0;
  int num_delete_failed = 0;
  for (const base::FilePath& path : paths) {
    if (!base::PathExists(path))
      continue;

    num_delete_attempted++;
    DCHECK(!base::DirectoryExists(path));

    if (!base::DeleteFile(path, false /* recursive */)) {
      num_delete_failed++;
    }
  }

  if (reason.has_value()) {
    stats::LogFileCleanupStatus(reason.value(), num_delete_attempted);
  }

  if (num_delete_failed > 0) {
    stats::LogFileCleanupStatus(stats::FileCleanupReason::DELETE_FAILED,
                                num_delete_failed);
  }
}

bool FileMonitor::ReadyForCleanup(const Entry* entry) {
  return entry->state == Entry::State::WATCHDOG &&
         (base::Time::Now() - entry->completion_time) > file_keep_alive_time_;
}

}  // namespace download
