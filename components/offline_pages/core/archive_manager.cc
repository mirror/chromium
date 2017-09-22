// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/archive_manager.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_macros.h"
#include "base/sequenced_task_runner.h"
#include "base/sys_info.h"
#include "base/threading/thread_task_runner_handle.h"

namespace offline_pages {

namespace {

using StorageStatsCallback =
    base::Callback<void(const ArchiveManager::StorageStats& storage_stats)>;

void EnsureArchivesDirCreatedImpl(const base::FilePath& archives_dir,
                                  bool is_temp) {
  base::File::Error error = base::File::FILE_OK;
  if (!base::DirectoryExists(archives_dir)) {
    if (!base::CreateDirectoryAndGetError(archives_dir, &error)) {
      LOG(ERROR) << "Failed to create offline pages archive directory: "
                 << base::File::ErrorToString(error);
    }
    std::string histogram_name(
        "OfflinePages.ArchiveManager.ArchiveDirsCreationResult");
    if (is_temp)
      histogram_name += ".temporary";
    else
      histogram_name += ".persistent";
    UMA_HISTOGRAM_ENUMERATION(histogram_name, -error,
                              -base::File::FILE_ERROR_MAX);
  }
}

void ExistsArchiveImpl(const base::FilePath& file_path,
                       scoped_refptr<base::SequencedTaskRunner> task_runner,
                       const base::Callback<void(bool)>& callback) {
  task_runner->PostTask(FROM_HERE,
                        base::Bind(callback, base::PathExists(file_path)));
}

void DeleteArchivesImpl(const std::vector<base::FilePath>& file_paths,
                        scoped_refptr<base::SequencedTaskRunner> task_runner,
                        const base::Callback<void(bool)>& callback) {
  bool result = false;
  for (const auto& file_path : file_paths) {
    // Make sure delete happens on the left of || so that it is always executed.
    result = base::DeleteFile(file_path, false) || result;
  }
  task_runner->PostTask(FROM_HERE, base::Bind(callback, result));
}

void GetAllArchivesImpl(
    const std::vector<base::FilePath>& archives_dirs,
    scoped_refptr<base::SequencedTaskRunner> task_runner,
    const base::Callback<void(const std::set<base::FilePath>&)>& callback) {
  std::set<base::FilePath> archive_paths;
  for (const auto& archives_dir : archives_dirs) {
    base::FileEnumerator file_enumerator(archives_dir, false,
                                         base::FileEnumerator::FILES);
    for (base::FilePath archive_path = file_enumerator.Next();
         !archive_path.empty(); archive_path = file_enumerator.Next()) {
      archive_paths.insert(archive_path);
    }
  }
  task_runner->PostTask(FROM_HERE, base::Bind(callback, archive_paths));
}

void GetStorageStatsImpl(const std::vector<base::FilePath>& archives_dirs,
                         scoped_refptr<base::SequencedTaskRunner> task_runner,
                         const StorageStatsCallback& callback) {
  ArchiveManager::StorageStats storage_stats;
  if (archives_dirs.size() == 0) {
    storage_stats.free_disk_space = -1;
    storage_stats.total_archives_size = -1;
    task_runner->PostTask(FROM_HERE, base::Bind(callback, storage_stats));
    return;
  }
  // Since both archive directories will be on the internal storage, the free
  // disk space can be acquired by using either of the directories.
  storage_stats.free_disk_space =
      base::SysInfo::AmountOfFreeDiskSpace(archives_dirs[0]);
  storage_stats.total_archives_size = 0;
  // DCHECK that if there are temporary and persistent directories, they should
  // be on the same volume. The assumption is because persistent offline pages
  // are saved within Chrome app data directory, which is on the same volumn as
  // Chrome's cache directory.
  // If in the future persistent pages will be moved to external storage, the
  // DCHECK will be violated, and it means this method needs to be updated with
  // getting StorageStats separately from internal and external storage.
  DCHECK(archives_dirs.size() < 2 ||
         base::SysInfo::AmountOfFreeDiskSpace(archives_dirs[1]) ==
             storage_stats.free_disk_space);
  for (const auto& archives_dir : archives_dirs)
    storage_stats.total_archives_size +=
        base::ComputeDirectorySize(archives_dir);
  task_runner->PostTask(FROM_HERE, base::Bind(callback, storage_stats));
}

}  // namespace

// protected and used for testing.
ArchiveManager::ArchiveManager() {}

ArchiveManager::ArchiveManager(
    const base::FilePath& temporary_archives_dir,
    const base::FilePath& persistent_archives_dir,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner)
    : temporary_archives_dir_(temporary_archives_dir),
      persistent_archives_dir_(persistent_archives_dir),
      task_runner_(task_runner) {}

ArchiveManager::~ArchiveManager() {}

void ArchiveManager::EnsureArchivesDirCreated(const base::Closure& callback) {
  // Since the task_runner is a SequencedTaskRunner, it's guaranteed that the
  // second task will start after the first one, so the callback will only be
  // invoked once both directories are created.
  if (!temporary_archives_dir_.empty())
    task_runner_->PostTask(
        FROM_HERE, base::Bind(EnsureArchivesDirCreatedImpl,
                              temporary_archives_dir_, true /* is_temp */));
  task_runner_->PostTaskAndReply(
      FROM_HERE,
      base::Bind(EnsureArchivesDirCreatedImpl, persistent_archives_dir_,
                 false /* is_temp */),
      callback);
}

void ArchiveManager::ExistsArchive(const base::FilePath& archive_path,
                                   const base::Callback<void(bool)>& callback) {
  task_runner_->PostTask(
      FROM_HERE, base::Bind(ExistsArchiveImpl, archive_path,
                            base::ThreadTaskRunnerHandle::Get(), callback));
}

void ArchiveManager::DeleteArchive(const base::FilePath& archive_path,
                                   const base::Callback<void(bool)>& callback) {
  std::vector<base::FilePath> archive_paths = {archive_path};
  DeleteMultipleArchives(archive_paths, callback);
}

void ArchiveManager::DeleteMultipleArchives(
    const std::vector<base::FilePath>& archive_paths,
    const base::Callback<void(bool)>& callback) {
  task_runner_->PostTask(
      FROM_HERE, base::Bind(DeleteArchivesImpl, archive_paths,
                            base::ThreadTaskRunnerHandle::Get(), callback));
}

void ArchiveManager::GetAllArchives(
    const base::Callback<void(const std::set<base::FilePath>&)>& callback)
    const {
  std::vector<base::FilePath> archives_dirs = {persistent_archives_dir_};
  if (!temporary_archives_dir_.empty())
    archives_dirs.push_back(temporary_archives_dir_);
  task_runner_->PostTask(
      FROM_HERE, base::Bind(GetAllArchivesImpl, archives_dirs,
                            base::ThreadTaskRunnerHandle::Get(), callback));
}

void ArchiveManager::GetStorageStats(
    const StorageStatsCallback& callback) const {
  std::vector<base::FilePath> archives_dirs = {persistent_archives_dir_};
  if (!temporary_archives_dir_.empty())
    archives_dirs.push_back(temporary_archives_dir_);
  task_runner_->PostTask(
      FROM_HERE, base::Bind(GetStorageStatsImpl, archives_dirs,
                            base::ThreadTaskRunnerHandle::Get(), callback));
}

void ArchiveManager::GetTemporaryStorageStats(
    const StorageStatsCallback& callback) const {
  std::vector<base::FilePath> archives_dirs;
  if (!temporary_archives_dir_.empty())
    archives_dirs.push_back(temporary_archives_dir_);
  task_runner_->PostTask(
      FROM_HERE, base::Bind(GetStorageStatsImpl, archives_dirs,
                            base::ThreadTaskRunnerHandle::Get(), callback));
}

const base::FilePath& ArchiveManager::GetTemporaryArchivesDir() const {
  return temporary_archives_dir_;
}

const base::FilePath& ArchiveManager::GetPersistentArchivesDir() const {
  return persistent_archives_dir_;
}

}  // namespace offline_pages
