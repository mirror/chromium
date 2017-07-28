// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/recent_async_file_util.h"

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "chrome/browser/chromeos/file_manager/volume_manager.h"
#include "chrome/browser/chromeos/fileapi/recent_context.h"
#include "chrome/browser/chromeos/fileapi/recent_file.h"
#include "chrome/browser/chromeos/fileapi/recent_util.h"
#include "content/public/browser/browser_thread.h"
#include "storage/browser/blob/shareable_file_reference.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation_context.h"
#include "storage/browser/fileapi/file_system_operation_runner.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/common/fileapi/directory_entry.h"

using content::BrowserThread;

class Profile;

namespace chromeos {

RecentAsyncFileUtil::RecentAsyncFileUtil(RecentModel* model)
    : model_(model), weak_ptr_factory_(this) {}

RecentAsyncFileUtil::~RecentAsyncFileUtil() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
}

void RecentAsyncFileUtil::CreateOrOpen(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    int file_flags,
    const CreateOrOpenCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // TODO(nya): Implement this function if it is ever called.
  NOTIMPLEMENTED();
  callback.Run(base::File(base::File::FILE_ERROR_INVALID_OPERATION),
               base::Closure());
}

void RecentAsyncFileUtil::EnsureFileExists(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    const EnsureFileExistsCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTREACHED();  // Read-only file system.
  callback.Run(base::File::FILE_ERROR_ACCESS_DENIED, false);
}

void RecentAsyncFileUtil::CreateDirectory(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    bool exclusive,
    bool recursive,
    const StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTREACHED();  // Read-only file system.
  callback.Run(base::File::FILE_ERROR_ACCESS_DENIED);
}

void RecentAsyncFileUtil::GetFileInfo(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    int fields,
    const GetFileInfoCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK_EQ(storage::kFileSystemTypeRecent, url.type());

  ParseRecentUrlOnIOThread(
      context->file_system_context(), url,
      base::BindOnce(&RecentAsyncFileUtil::GetFileInfoWithContext,
                     weak_ptr_factory_.GetWeakPtr(),
                     make_scoped_refptr(context->file_system_context()), fields,
                     callback));
}

void RecentAsyncFileUtil::ReadDirectory(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    const ReadDirectoryCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK_EQ(storage::kFileSystemTypeRecent, url.type());

  ParseRecentUrlOnIOThread(
      context->file_system_context(), url,
      base::BindOnce(&RecentAsyncFileUtil::ReadDirectoryWithContext,
                     weak_ptr_factory_.GetWeakPtr(),
                     make_scoped_refptr(context->file_system_context()),
                     callback));
}

void RecentAsyncFileUtil::Touch(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    const base::Time& last_access_time,
    const base::Time& last_modified_time,
    const StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTREACHED();  // Read-only file system.
  callback.Run(base::File::FILE_ERROR_ACCESS_DENIED);
}

void RecentAsyncFileUtil::Truncate(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    int64_t length,
    const StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTREACHED();  // Read-only file system.
  callback.Run(base::File::FILE_ERROR_ACCESS_DENIED);
}

void RecentAsyncFileUtil::CopyFileLocal(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& src_url,
    const storage::FileSystemURL& dest_url,
    CopyOrMoveOption option,
    const CopyFileProgressCallback& progress_callback,
    const StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTREACHED();  // Read-only file system.
  callback.Run(base::File::FILE_ERROR_ACCESS_DENIED);
}

void RecentAsyncFileUtil::MoveFileLocal(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& src_url,
    const storage::FileSystemURL& dest_url,
    CopyOrMoveOption option,
    const StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTREACHED();  // Read-only file system.
  callback.Run(base::File::FILE_ERROR_ACCESS_DENIED);
}

void RecentAsyncFileUtil::CopyInForeignFile(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const base::FilePath& src_file_path,
    const storage::FileSystemURL& dest_url,
    const StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTREACHED();  // Read-only file system.
  callback.Run(base::File::FILE_ERROR_ACCESS_DENIED);
}

void RecentAsyncFileUtil::DeleteFile(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    const StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTREACHED();  // Read-only file system.
  callback.Run(base::File::FILE_ERROR_ACCESS_DENIED);
}

void RecentAsyncFileUtil::DeleteDirectory(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    const StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTREACHED();  // Read-only file system.
  callback.Run(base::File::FILE_ERROR_ACCESS_DENIED);
}

void RecentAsyncFileUtil::DeleteRecursively(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    const StatusCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTREACHED();  // Read-only file system.
  callback.Run(base::File::FILE_ERROR_ACCESS_DENIED);
}

void RecentAsyncFileUtil::CreateSnapshotFile(
    std::unique_ptr<storage::FileSystemOperationContext> context,
    const storage::FileSystemURL& url,
    const CreateSnapshotFileCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  NOTIMPLEMENTED();  // TODO(crbug.com/671511): Implement this function.
  callback.Run(base::File::FILE_ERROR_FAILED, base::File::Info(),
               base::FilePath(),
               scoped_refptr<storage::ShareableFileReference>());
}

void RecentAsyncFileUtil::GetFileInfoWithContext(
    scoped_refptr<storage::FileSystemContext> file_system_context,
    int fields,
    const GetFileInfoCallback& callback,
    RecentContext context,
    const base::FilePath& path) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!context.is_valid()) {
    callback.Run(base::File::FILE_ERROR_SECURITY, base::File::Info());
    return;
  }

  // The root directory is a fake entry.
  if (path.empty()) {
    base::File::Info info;
    info.size = -1;
    info.is_directory = true;
    info.is_symbolic_link = false;
    info.last_modified = info.last_accessed = info.creation_time =
        base::Time::UnixEpoch();  // arbitrary
    callback.Run(base::File::FILE_OK, info);
    return;
  }

  model_->GetFilesMap(
      context, false /* want_refresh */,
      base::BindOnce(&RecentAsyncFileUtil::GetFileInfoWithFilesMap,
                     weak_ptr_factory_.GetWeakPtr(), file_system_context,
                     fields, callback, path));
}

void RecentAsyncFileUtil::GetFileInfoWithFilesMap(
    scoped_refptr<storage::FileSystemContext> file_system_context,
    int fields,
    const GetFileInfoCallback& callback,
    const base::FilePath& path,
    const RecentModel::FilesMap& files_map) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  auto iter = files_map.find(path);
  if (iter == files_map.end()) {
    callback.Run(base::File::FILE_ERROR_NOT_FOUND, base::File::Info());
    return;
  }

  RecentFile* file = iter->second.get();
  file->GetFileInfo(fields, file_system_context.get(), callback);
}

void RecentAsyncFileUtil::ReadDirectoryWithContext(
    scoped_refptr<storage::FileSystemContext> file_system_context,
    const ReadDirectoryCallback& callback,
    RecentContext context,
    const base::FilePath& path) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (!context.is_valid()) {
    callback.Run(base::File::FILE_ERROR_SECURITY, EntryList(), false);
    return;
  }

  if (!path.empty()) {
    callback.Run(base::File::FILE_ERROR_NOT_A_DIRECTORY, EntryList(), false);
    return;
  }

  model_->GetFilesMap(
      context, true /* want_refresh */,
      base::BindOnce(&RecentAsyncFileUtil::ReadDirectoryWithFilesMap,
                     weak_ptr_factory_.GetWeakPtr(), callback));
}

void RecentAsyncFileUtil::ReadDirectoryWithFilesMap(
    const ReadDirectoryCallback& callback,
    const RecentModel::FilesMap& files_map) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  EntryList entries;
  for (const auto& pair : files_map) {
    entries.emplace_back(storage::DirectoryEntry(
        pair.first.AsUTF8Unsafe(), storage::DirectoryEntry::FILE));
  }
  callback.Run(base::File::FILE_OK, entries, false);
}

}  // namespace chromeos
