// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "storage/browser/fileapi/file_system_operation_runner.h"

#include <stdint.h>

#include <memory>
#include <tuple>
#include <utility>

#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/stl_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/url_request/url_request_context.h"
#include "storage/browser/blob/blob_url_request_job_factory.h"
#include "storage/browser/blob/shareable_file_reference.h"
#include "storage/browser/fileapi/file_observers.h"
#include "storage/browser/fileapi/file_stream_writer.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_writer_delegate.h"

namespace storage {

typedef FileSystemOperationRunner::OperationID OperationID;

class FileSystemOperationRunner::BeginOperationScoper
    : public base::SupportsWeakPtr<
          FileSystemOperationRunner::BeginOperationScoper> {
 public:
  BeginOperationScoper() {}
 private:
  DISALLOW_COPY_AND_ASSIGN(BeginOperationScoper);
};

FileSystemOperationRunner::OperationHandle::OperationHandle() {}
FileSystemOperationRunner::OperationHandle::OperationHandle(
    const OperationHandle& other) = default;
FileSystemOperationRunner::OperationHandle::~OperationHandle() {}

FileSystemOperationRunner::~FileSystemOperationRunner() {
}

void FileSystemOperationRunner::Shutdown() {
  operations_.Clear();
}

OperationID FileSystemOperationRunner::CreateFile(const FileSystemURL& url,
                                                  bool exclusive,
                                                  StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->CreateFile(
      url, exclusive,
      base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(), handle,
                     std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::CreateDirectory(
    const FileSystemURL& url,
    bool exclusive,
    bool recursive,
    StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->CreateDirectory(
      url, exclusive, recursive,
      base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(), handle,
                     std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::Copy(
    const FileSystemURL& src_url,
    const FileSystemURL& dest_url,
    CopyOrMoveOption option,
    ErrorBehavior error_behavior,
    CopyProgressCallback progress_callback,
    StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(dest_url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForWrite(handle.id, dest_url);
  PrepareForRead(handle.id, src_url);
  operation_raw->Copy(
      src_url, dest_url, option, error_behavior,
      progress_callback.is_null()
          ? CopyProgressCallback()
          : base::BindRepeating(&FileSystemOperationRunner::OnCopyProgress,
                                AsWeakPtr(), handle,
                                std::move(progress_callback)),
      base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(), handle,
                     std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::Move(const FileSystemURL& src_url,
                                            const FileSystemURL& dest_url,
                                            CopyOrMoveOption option,
                                            StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(dest_url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForWrite(handle.id, dest_url);
  PrepareForWrite(handle.id, src_url);
  operation_raw->Move(src_url, dest_url, option,
                      base::BindOnce(&FileSystemOperationRunner::DidFinish,
                                     AsWeakPtr(), handle, std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::DirectoryExists(
    const FileSystemURL& url,
    StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForRead(handle.id, url);
  operation_raw->DirectoryExists(
      url, base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                          handle, std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::FileExists(const FileSystemURL& url,
                                                  StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForRead(handle.id, url);
  operation_raw->FileExists(
      url, base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                          handle, std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::GetMetadata(
    const FileSystemURL& url,
    int fields,
    GetMetadataCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidGetMetadata(handle, std::move(callback), error, base::File::Info());
    return handle.id;
  }
  PrepareForRead(handle.id, url);
  operation_raw->GetMetadata(
      url, fields,
      base::BindOnce(&FileSystemOperationRunner::DidGetMetadata, AsWeakPtr(),
                     handle, std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::ReadDirectory(
    const FileSystemURL& url,
    ReadDirectoryCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidReadDirectory(handle, std::move(callback), error,
                     std::vector<DirectoryEntry>(), false);
    return handle.id;
  }
  PrepareForRead(handle.id, url);
  operation_raw->ReadDirectory(
      url, base::BindRepeating(&FileSystemOperationRunner::DidReadDirectory,
                               AsWeakPtr(), handle, std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::Remove(const FileSystemURL& url,
                                              bool recursive,
                                              StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->Remove(
      url, recursive,
      base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(), handle,
                     std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::Write(
    const net::URLRequestContext* url_request_context,
    const FileSystemURL& url,
    std::unique_ptr<storage::BlobDataHandle> blob,
    int64_t offset,
    WriteCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidWrite(handle, std::move(callback), error, 0, true);
    return handle.id;
  }

  std::unique_ptr<FileStreamWriter> writer(
      file_system_context_->CreateFileStreamWriter(url, offset));
  if (!writer) {
    // Write is not supported.
    DidWrite(handle, std::move(callback), base::File::FILE_ERROR_SECURITY, 0,
             true);
    return handle.id;
  }

  std::unique_ptr<FileWriterDelegate> writer_delegate(new FileWriterDelegate(
      std::move(writer), url.mount_option().flush_policy()));

  std::unique_ptr<net::URLRequest> blob_request(
      storage::BlobProtocolHandler::CreateBlobRequest(
          std::move(blob), url_request_context, writer_delegate.get()));

  PrepareForWrite(handle.id, url);
  operation_raw->Write(
      url, std::move(writer_delegate), std::move(blob_request),
      base::BindRepeating(&FileSystemOperationRunner::DidWrite, AsWeakPtr(),
                          handle, std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::Truncate(const FileSystemURL& url,
                                                int64_t length,
                                                StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->Truncate(
      url, length,
      base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(), handle,
                     std::move(callback)));
  return handle.id;
}

void FileSystemOperationRunner::Cancel(OperationID id,
                                       StatusCallback callback) {
  if (base::ContainsKey(finished_operations_, id)) {
    DCHECK(!base::ContainsKey(stray_cancel_callbacks_, id));
    stray_cancel_callbacks_[id] = std::move(callback);
    return;
  }
  FileSystemOperation* operation = operations_.Lookup(id);
  if (!operation) {
    // There is no operation with |id|.
    std::move(callback).Run(base::File::FILE_ERROR_INVALID_OPERATION);
    return;
  }
  operation->Cancel(std::move(callback));
}

OperationID FileSystemOperationRunner::TouchFile(
    const FileSystemURL& url,
    const base::Time& last_access_time,
    const base::Time& last_modified_time,
    StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->TouchFile(
      url, last_access_time, last_modified_time,
      base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(), handle,
                     std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::OpenFile(const FileSystemURL& url,
                                                int file_flags,
                                                OpenFileCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidOpenFile(handle, std::move(callback), base::File(error),
                base::Closure());
    return handle.id;
  }
  if (file_flags &
      (base::File::FLAG_CREATE | base::File::FLAG_OPEN_ALWAYS |
       base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_OPEN_TRUNCATED |
       base::File::FLAG_WRITE | base::File::FLAG_EXCLUSIVE_WRITE |
       base::File::FLAG_DELETE_ON_CLOSE |
       base::File::FLAG_WRITE_ATTRIBUTES)) {
    PrepareForWrite(handle.id, url);
  } else {
    PrepareForRead(handle.id, url);
  }
  operation_raw->OpenFile(
      url, file_flags,
      base::BindOnce(&FileSystemOperationRunner::DidOpenFile, AsWeakPtr(),
                     handle, std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::CreateSnapshotFile(
    const FileSystemURL& url,
    SnapshotFileCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidCreateSnapshot(handle, std::move(callback), error, base::File::Info(),
                      base::FilePath(), NULL);
    return handle.id;
  }
  PrepareForRead(handle.id, url);
  operation_raw->CreateSnapshotFile(
      url, base::BindOnce(&FileSystemOperationRunner::DidCreateSnapshot,
                          AsWeakPtr(), handle, std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::CopyInForeignFile(
    const base::FilePath& src_local_disk_path,
    const FileSystemURL& dest_url,
    StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(dest_url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForWrite(handle.id, dest_url);
  operation_raw->CopyInForeignFile(
      src_local_disk_path, dest_url,
      base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(), handle,
                     std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::RemoveFile(const FileSystemURL& url,
                                                  StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->RemoveFile(
      url, base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                          handle, std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::RemoveDirectory(
    const FileSystemURL& url,
    StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForWrite(handle.id, url);
  operation_raw->RemoveDirectory(
      url, base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                          handle, std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::CopyFileLocal(
    const FileSystemURL& src_url,
    const FileSystemURL& dest_url,
    CopyOrMoveOption option,
    CopyFileProgressCallback progress_callback,
    StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(src_url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForRead(handle.id, src_url);
  PrepareForWrite(handle.id, dest_url);
  operation_raw->CopyFileLocal(
      src_url, dest_url, option, std::move(progress_callback),
      base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(), handle,
                     std::move(callback)));
  return handle.id;
}

OperationID FileSystemOperationRunner::MoveFileLocal(
    const FileSystemURL& src_url,
    const FileSystemURL& dest_url,
    CopyOrMoveOption option,
    StatusCallback callback) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation = base::WrapUnique(
      file_system_context_->CreateFileSystemOperation(src_url, &error));
  FileSystemOperation* operation_raw = operation.get();
  BeginOperationScoper scope;
  OperationHandle handle =
      BeginOperation(std::move(operation), scope.AsWeakPtr());
  if (!operation_raw) {
    DidFinish(handle, std::move(callback), error);
    return handle.id;
  }
  PrepareForWrite(handle.id, src_url);
  PrepareForWrite(handle.id, dest_url);
  operation_raw->MoveFileLocal(
      src_url, dest_url, option,
      base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(), handle,
                     std::move(callback)));
  return handle.id;
}

base::File::Error FileSystemOperationRunner::SyncGetPlatformPath(
    const FileSystemURL& url,
    base::FilePath* platform_path) {
  base::File::Error error = base::File::FILE_OK;
  std::unique_ptr<FileSystemOperation> operation(
      file_system_context_->CreateFileSystemOperation(url, &error));
  if (!operation.get())
    return error;
  return operation->SyncGetPlatformPath(url, platform_path);
}

FileSystemOperationRunner::FileSystemOperationRunner(
    FileSystemContext* file_system_context)
    : file_system_context_(file_system_context) {}

void FileSystemOperationRunner::DidFinish(const OperationHandle& handle,
                                          StatusCallback callback,
                                          base::File::Error rv) {
  if (handle.scope) {
    finished_operations_.insert(handle.id);
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&FileSystemOperationRunner::DidFinish, AsWeakPtr(),
                       handle, std::move(callback), rv));
    return;
  }
  std::move(callback).Run(rv);
  FinishOperation(handle.id);
}

void FileSystemOperationRunner::DidGetMetadata(
    const OperationHandle& handle,
    GetMetadataCallback callback,
    base::File::Error rv,
    const base::File::Info& file_info) {
  if (handle.scope) {
    finished_operations_.insert(handle.id);
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&FileSystemOperationRunner::DidGetMetadata, AsWeakPtr(),
                       handle, std::move(callback), rv, file_info));
    return;
  }
  std::move(callback).Run(rv, file_info);
  FinishOperation(handle.id);
}

void FileSystemOperationRunner::DidReadDirectory(
    const OperationHandle& handle,
    ReadDirectoryCallback callback,
    base::File::Error rv,
    std::vector<DirectoryEntry> entries,
    bool has_more) {
  if (handle.scope) {
    finished_operations_.insert(handle.id);
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&FileSystemOperationRunner::DidReadDirectory,
                                  AsWeakPtr(), handle, std::move(callback), rv,
                                  std::move(entries), has_more));
    return;
  }
  std::move(callback).Run(rv, std::move(entries), has_more);
  if (rv != base::File::FILE_OK || !has_more)
    FinishOperation(handle.id);
}

void FileSystemOperationRunner::DidWrite(const OperationHandle& handle,
                                         WriteCallback callback,
                                         base::File::Error rv,
                                         int64_t bytes,
                                         bool complete) {
  if (handle.scope) {
    finished_operations_.insert(handle.id);
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&FileSystemOperationRunner::DidWrite, AsWeakPtr(),
                       handle, std::move(callback), rv, bytes, complete));
    return;
  }
  std::move(callback).Run(rv, bytes, complete);
  if (rv != base::File::FILE_OK || complete)
    FinishOperation(handle.id);
}

void FileSystemOperationRunner::DidOpenFile(
    const OperationHandle& handle,
    OpenFileCallback callback,
    base::File file,
    base::OnceClosure on_close_callback) {
  if (handle.scope) {
    finished_operations_.insert(handle.id);
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&FileSystemOperationRunner::DidOpenFile, AsWeakPtr(),
                       handle, std::move(callback), std::move(file),
                       std::move(on_close_callback)));
    return;
  }
  std::move(callback).Run(std::move(file), std::move(on_close_callback));
  FinishOperation(handle.id);
}

void FileSystemOperationRunner::DidCreateSnapshot(
    const OperationHandle& handle,
    SnapshotFileCallback callback,
    base::File::Error rv,
    const base::File::Info& file_info,
    const base::FilePath& platform_path,
    scoped_refptr<storage::ShareableFileReference> file_ref) {
  if (handle.scope) {
    finished_operations_.insert(handle.id);
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&FileSystemOperationRunner::DidCreateSnapshot,
                       AsWeakPtr(), handle, std::move(callback), rv, file_info,
                       platform_path, std::move(file_ref)));
    return;
  }
  std::move(callback).Run(rv, file_info, platform_path, std::move(file_ref));
  FinishOperation(handle.id);
}

void FileSystemOperationRunner::OnCopyProgress(
    const OperationHandle& handle,
    CopyProgressCallback callback,
    FileSystemOperation::CopyProgressType type,
    const FileSystemURL& source_url,
    const FileSystemURL& dest_url,
    int64_t size) {
  if (handle.scope) {
    base::ThreadTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&FileSystemOperationRunner::OnCopyProgress,
                                  AsWeakPtr(), handle, std::move(callback),
                                  type, source_url, dest_url, size));
    return;
  }
  std::move(callback).Run(type, source_url, dest_url, size);
}

void FileSystemOperationRunner::PrepareForWrite(OperationID id,
                                                const FileSystemURL& url) {
  if (file_system_context_->GetUpdateObservers(url.type())) {
    file_system_context_->GetUpdateObservers(url.type())->Notify(
        &FileUpdateObserver::OnStartUpdate, std::make_tuple(url));
  }
  write_target_urls_[id].insert(url);
}

void FileSystemOperationRunner::PrepareForRead(OperationID id,
                                               const FileSystemURL& url) {
  if (file_system_context_->GetAccessObservers(url.type())) {
    file_system_context_->GetAccessObservers(url.type())->Notify(
        &FileAccessObserver::OnAccess, std::make_tuple(url));
  }
}

FileSystemOperationRunner::OperationHandle
FileSystemOperationRunner::BeginOperation(
    std::unique_ptr<FileSystemOperation> operation,
    base::WeakPtr<BeginOperationScoper> scope) {
  OperationHandle handle;
  handle.id = operations_.Add(std::move(operation));
  handle.scope = scope;
  return handle;
}

void FileSystemOperationRunner::FinishOperation(OperationID id) {
  OperationToURLSet::iterator found = write_target_urls_.find(id);
  if (found != write_target_urls_.end()) {
    const FileSystemURLSet& urls = found->second;
    for (FileSystemURLSet::const_iterator iter = urls.begin();
        iter != urls.end(); ++iter) {
      if (file_system_context_->GetUpdateObservers(iter->type())) {
        file_system_context_->GetUpdateObservers(iter->type())->Notify(
            &FileUpdateObserver::OnEndUpdate, std::make_tuple(*iter));
      }
    }
    write_target_urls_.erase(found);
  }

  // IDMap::Lookup fails if the operation is NULL, so we don't check
  // operations_.Lookup(id) here.

  operations_.Remove(id);
  finished_operations_.erase(id);

  // Dispatch stray cancel callback if exists.
  std::map<OperationID, StatusCallback>::iterator found_cancel =
      stray_cancel_callbacks_.find(id);
  if (found_cancel != stray_cancel_callbacks_.end()) {
    // This cancel has been requested after the operation has finished,
    // so report that we failed to stop it.
    std::move(found_cancel->second)
        .Run(base::File::FILE_ERROR_INVALID_OPERATION);
    stray_cancel_callbacks_.erase(found_cancel);
  }
}

}  // namespace storage
