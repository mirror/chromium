// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef STORAGE_BROWSER_FILEAPI_ASYNC_FILE_UTIL_ADAPTER_H_
#define STORAGE_BROWSER_FILEAPI_ASYNC_FILE_UTIL_ADAPTER_H_

#include "base/memory/scoped_ptr.h"
#include "storage/browser/fileapi/async_file_util.h"

namespace storage {

class FileSystemFileUtil;

// An adapter class for FileSystemFileUtil classes to provide asynchronous
// interface.
//
// A filesystem can do either:
// - implement a synchronous version of FileUtil by extending
//   FileSystemFileUtil and atach it to this adapter, or
// - directly implement AsyncFileUtil.
//
// This instance (as thus this->sync_file_util_) is guaranteed to be alive
// as far as FileSystemOperationContext given to each operation is kept alive.
class STORAGE_EXPORT AsyncFileUtilAdapter
    : public NON_EXPORTED_BASE(AsyncFileUtil) {
 public:
  // Creates a new AsyncFileUtil for |sync_file_util|. This takes the
  // ownership of |sync_file_util|. (This doesn't take scoped_ptr<> just
  // to save extra make_scoped_ptr; in all use cases a new fresh FileUtil is
  // created only for this adapter.)
  explicit AsyncFileUtilAdapter(FileSystemFileUtil* sync_file_util);

  virtual ~AsyncFileUtilAdapter();

  FileSystemFileUtil* sync_file_util() {
    return sync_file_util_.get();
  }

  // AsyncFileUtil overrides.
  virtual void CreateOrOpen(
      scoped_ptr<FileSystemOperationContext> context,
      const FileSystemURL& url,
      int file_flags,
      const CreateOrOpenCallback& callback) override;
  virtual void EnsureFileExists(
      scoped_ptr<FileSystemOperationContext> context,
      const FileSystemURL& url,
      const EnsureFileExistsCallback& callback) override;
  virtual void CreateDirectory(
      scoped_ptr<FileSystemOperationContext> context,
      const FileSystemURL& url,
      bool exclusive,
      bool recursive,
      const StatusCallback& callback) override;
  virtual void GetFileInfo(
      scoped_ptr<FileSystemOperationContext> context,
      const FileSystemURL& url,
      const GetFileInfoCallback& callback) override;
  virtual void ReadDirectory(
      scoped_ptr<FileSystemOperationContext> context,
      const FileSystemURL& url,
      const ReadDirectoryCallback& callback) override;
  virtual void Touch(
      scoped_ptr<FileSystemOperationContext> context,
      const FileSystemURL& url,
      const base::Time& last_access_time,
      const base::Time& last_modified_time,
      const StatusCallback& callback) override;
  virtual void Truncate(
      scoped_ptr<FileSystemOperationContext> context,
      const FileSystemURL& url,
      int64 length,
      const StatusCallback& callback) override;
  virtual void CopyFileLocal(
      scoped_ptr<FileSystemOperationContext> context,
      const FileSystemURL& src_url,
      const FileSystemURL& dest_url,
      CopyOrMoveOption option,
      const CopyFileProgressCallback& progress_callback,
      const StatusCallback& callback) override;
  virtual void MoveFileLocal(
      scoped_ptr<FileSystemOperationContext> context,
      const FileSystemURL& src_url,
      const FileSystemURL& dest_url,
      CopyOrMoveOption option,
      const StatusCallback& callback) override;
  virtual void CopyInForeignFile(
      scoped_ptr<FileSystemOperationContext> context,
      const base::FilePath& src_file_path,
      const FileSystemURL& dest_url,
      const StatusCallback& callback) override;
  virtual void DeleteFile(
      scoped_ptr<FileSystemOperationContext> context,
      const FileSystemURL& url,
      const StatusCallback& callback) override;
  virtual void DeleteDirectory(
      scoped_ptr<FileSystemOperationContext> context,
      const FileSystemURL& url,
      const StatusCallback& callback) override;
  virtual void DeleteRecursively(
      scoped_ptr<FileSystemOperationContext> context,
      const FileSystemURL& url,
      const StatusCallback& callback) override;
  virtual void CreateSnapshotFile(
      scoped_ptr<FileSystemOperationContext> context,
      const FileSystemURL& url,
      const CreateSnapshotFileCallback& callback) override;

 private:
  scoped_ptr<FileSystemFileUtil> sync_file_util_;

  DISALLOW_COPY_AND_ASSIGN(AsyncFileUtilAdapter);
};

}  // namespace storage

#endif  // STORAGE_BROWSER_FILEAPI_ASYNC_FILE_UTIL_ADAPTER_H_
