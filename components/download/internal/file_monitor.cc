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
#include "base/threading/thread_restrictions.h"
#include "components/download/internal/entry.h"

namespace download {

FileMonitor::FileMonitor(
    const base::FilePath& download_dir,
    const scoped_refptr<base::SequencedTaskRunner>& file_thread_task_runner,
    base::TimeDelta file_keep_alive_time)
    : download_dir_(download_dir),
      file_thread_task_runner_(file_thread_task_runner),
      file_keep_alive_time_(file_keep_alive_time),
      weak_factory_(this) {}

FileMonitor::~FileMonitor() = default;

void FileMonitor::RemoveUnassociatedFiles(
    const Model::EntryList& entries,
    const std::vector<DriverEntry>& driver_entries) {
  std::set<base::FilePath> download_file_paths;
  for (Entry* entry : entries) {
    download_file_paths.insert(entry->target_file_path);
  }

  for (const DriverEntry& driver_entry : driver_entries) {
    download_file_paths.insert(driver_entry.physical_file_path);
  }

  file_thread_task_runner_->PostTask(
      FROM_HERE, base::Bind(&FileMonitor::RemoveUnassociatedFilesOnFileThread,
                            weak_factory_.GetWeakPtr(), download_file_paths));
}

void FileMonitor::RemoveUnassociatedFilesOnFileThread(
    const std::set<base::FilePath>& download_file_paths) {
  base::ThreadRestrictions::AssertIOAllowed();
  std::set<base::FilePath> files_in_dir;
  base::FileEnumerator file_enumerator(download_dir_, false /* recursive */,
                                       base::FileEnumerator::FILES);

  for (base::FilePath path = file_enumerator.Next(); !path.value().empty();
       path = file_enumerator.Next()) {
    files_in_dir.insert(path);
  }

  std::vector<base::FilePath> files_to_remove =
      base::STLSetDifference<std::vector<base::FilePath>>(files_in_dir,
                                                          download_file_paths);
  DeleteFilesOnFileThread(files_to_remove, stats::FileCleanupReason::ORPHANED);
}

void FileMonitor::RemoveEntriesAfterTimeout(
    const Model::EntryList& entries,
    const RemoveEntriesCallback& callback) {
  std::vector<Entry*> entries_to_remove;
  std::vector<base::FilePath> files_to_remove;
  for (auto* entry : entries) {
    if (!ReadyForCleanup(entry))
      continue;

    entries_to_remove.push_back(entry);
    files_to_remove.push_back(entry->target_file_path);
  }

  file_thread_task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&FileMonitor::DeleteFilesOnFileThread,
                 weak_factory_.GetWeakPtr(), files_to_remove,
                 stats::FileCleanupReason::TIMEOUT),
      base::Bind(callback, entries_to_remove));
}

void FileMonitor::DeleteFilesOnFileThread(
    const std::vector<base::FilePath>& paths,
    const base::Optional<stats::FileCleanupReason>& reason) {
  base::ThreadRestrictions::AssertIOAllowed();
  int num_delete_attempted = 0;
  int num_delete_failed = 0;
  int num_delete_by_client = 0;
  for (const base::FilePath& path : paths) {
    if (!base::PathExists(path)) {
      if (reason == stats::FileCleanupReason::TIMEOUT)
        num_delete_by_client++;

      continue;
    }

    num_delete_attempted++;
    DCHECK(!base::DirectoryExists(path));

    if (!base::DeleteFile(path, false /* recursive */)) {
      num_delete_failed++;
    }
  }

  if (reason.has_value() && num_delete_attempted > 0) {
    stats::LogFileCleanupStatus(reason.value(), num_delete_attempted);
  }

  if (num_delete_by_client > 0) {
    stats::LogFileCleanupStatus(stats::FileCleanupReason::CLEANUP_BY_CLIENT,
                                num_delete_failed);
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
