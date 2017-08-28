// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/file_system_provider/smb_provided_file_system.h"

#include <stddef.h>

#include <algorithm>
#include <utility>

#include "base/memory/ptr_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/base/io_buffer.h"

namespace chromeos {
namespace file_system_provider {
namespace {

const char kFakeFileModificationTime[] = "Fri Apr 25 01:47:53 UTC 2014";
const char kFakeFileMimeType[] = "text/plain";
const int64_t kFakeFileSize = 200 * 1024 * 1024;
const int32_t kReadBlockSize = 64 * 1024;
const size_t kReadDirBatchSize = 64;

}  // namespace

const base::FilePath::CharType kFakeFilePath[] = FILE_PATH_LITERAL("/file.txt");

SmbProvidedFileSystem::SmbProvidedFileSystem(
    const ProvidedFileSystemInfo& file_system_info)
    : file_system_info_(file_system_info),
      last_file_handle_(0),
      weak_ptr_factory_(this) {
  PopulateEntries();
}

SmbProvidedFileSystem::~SmbProvidedFileSystem() {}

AbortCallback SmbProvidedFileSystem::RequestUnmount(
    const storage::AsyncFileUtil::StatusCallback& callback) {
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

AbortCallback SmbProvidedFileSystem::GetMetadata(
    const base::FilePath& entry_path,
    ProvidedFileSystemInterface::MetadataFieldMask fields,
    const ProvidedFileSystemInterface::GetMetadataCallback& callback) {
  // TODO(??):: Why are all the fields on the heap in EntryMetadata?

  // LOG(ERROR) << "SmbProvidedFileSystem::GetMetadata entry_path="
  //            << entry_path.value();
  std::unique_ptr<EntryMetadata> metadata(new EntryMetadata);
  bool is_dir = entry_path.BaseName().value().find("file") == std::string::npos;

  if (fields & ProvidedFileSystemInterface::METADATA_FIELD_IS_DIRECTORY) {
    metadata->is_directory.reset(new bool(is_dir));
  }

  if (fields & ProvidedFileSystemInterface::METADATA_FIELD_NAME)
    metadata->name.reset(new std::string(entry_path.BaseName().value()));

  if (fields & ProvidedFileSystemInterface::METADATA_FIELD_SIZE)
    metadata->size.reset(new int64_t(is_dir ? 0 : kFakeFileSize));

  if (fields & ProvidedFileSystemInterface::METADATA_FIELD_MODIFICATION_TIME) {
    base::Time modification_time;
    bool result =
        base::Time::FromString(kFakeFileModificationTime, &modification_time);
    DCHECK(result);
    metadata->modification_time.reset(new base::Time(modification_time));
  }
  if (fields & ProvidedFileSystemInterface::METADATA_FIELD_MIME_TYPE) {
    metadata->mime_type.reset(new std::string(kFakeFileMimeType));
  }
  if (fields & ProvidedFileSystemInterface::METADATA_FIELD_THUMBNAIL) {
    metadata->thumbnail.reset(new std::string("data:image/png;base64,X"));
  }

  return PostAbortableTask(
      base::Bind(callback, base::Passed(&metadata), base::File::FILE_OK));
}

AbortCallback SmbProvidedFileSystem::GetActions(
    const std::vector<base::FilePath>& entry_paths,
    const ProvidedFileSystemInterface::GetActionsCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  const std::vector<Action> actions;
  return PostAbortableTask(base::Bind(callback, actions, base::File::FILE_OK));
}

AbortCallback SmbProvidedFileSystem::ExecuteAction(
    const std::vector<base::FilePath>& entry_paths,
    const std::string& action_id,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

void SmbProvidedFileSystem::PopulateEntries() {
  size_t kFileCount = 10000;
  size_t kDirCount = 5;
  for (size_t i = 0; i < kDirCount; i++) {
    std::string name = std::string("dir_") + std::to_string(i);
    entry_list_.push_back(
        storage::DirectoryEntry(name, storage::DirectoryEntry::DIRECTORY));
  }

  for (size_t i = 0; i < kFileCount; i++) {
    std::string name = std::string("file_") + std::to_string(i);
    entry_list_.push_back(
        storage::DirectoryEntry(name, storage::DirectoryEntry::FILE));
  }
}
AbortCallback SmbProvidedFileSystem::ReadDirectory(
    const base::FilePath& directory_path,
    const storage::AsyncFileUtil::ReadDirectoryCallback& callback) {
  storage::AsyncFileUtil::EntryList entry_list;
  std::vector<int> task_ids;
  DCHECK(entry_list_.size() > 0);
  for (size_t i = 0; i < entry_list_.size(); i++) {
    entry_list.push_back(entry_list_[i]);

    if (i > 0 &&
        ((i == entry_list_.size() - 1) || (i % kReadDirBatchSize == 0))) {
      // LOG(ERROR) << "SmbProvidedFileSystem::ReadDirectory sending batch i="
      // << i
      //            << " size=" << entry_list.size();
      const int task_id = tracker_.PostTask(
          base::ThreadTaskRunnerHandle::Get().get(), FROM_HERE,
          base::BindOnce(callback, base::File::FILE_OK, entry_list,
                         (i != entry_list_.size() - 1) /* has_more */));
      task_ids.push_back(task_id);
      entry_list.clear();
    }
  }

  // LOG(ERROR) << "SmbProvidedFileSystem::ReadFile DONE";
  return base::Bind(&SmbProvidedFileSystem::AbortMany,
                    weak_ptr_factory_.GetWeakPtr(), task_ids);

  // TODO: Batching here?
  // return PostAbortableTask(base::Bind(callback, base::File::FILE_OK,
  // entry_list,
  //                                     false /* has_more */));
}

AbortCallback SmbProvidedFileSystem::OpenFile(
    const base::FilePath& entry_path,
    OpenFileMode mode,
    const OpenFileCallback& callback) {
  // LOG(ERROR) << "SmbProvidedFileSystem::OpenFile " << entry_path.value();

  const int file_handle = ++last_file_handle_;
  return PostAbortableTask(
      base::Bind(callback, file_handle, base::File::FILE_OK));
}

AbortCallback SmbProvidedFileSystem::CloseFile(
    int file_handle,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

AbortCallback SmbProvidedFileSystem::ReadFile(
    int file_handle,
    net::IOBuffer* buffer,
    int64_t offset,
    int length,
    const ProvidedFileSystemInterface::ReadChunkReceivedCallback& callback) {
  // LOG(ERROR) << "SmbProvidedFileSystem::ReadFile ofst=" << offset
  //            << " len=" << length;
  if (length <= 0 || offset >= kFakeFileSize) {
    return PostAbortableTask(base::Bind(callback, 0 /* chunk_length */,
                                        false /* has_more */,
                                        base::File::FILE_OK));
  }

  std::vector<int> task_ids;
  int64_t upto = offset;
  int remaining = std::min(length, (int)kFakeFileSize - (int)offset);
  int batch_size = kReadBlockSize;

  // If batch_size is 0, send it all at once.
  if (batch_size == 0) {
    batch_size = length;
  }

  do {
    int read_len = std::min(remaining, batch_size);
    // LOG(ERROR) << "SmbProvidedFileSystem::ReadFile read_len=" << read_len;
    memset(buffer->data(), 0, read_len);

    bool has_more = (upto + read_len) < (offset + length);
    // LOG(ERROR) << "SmbProvidedFileSystem::ReadFile has_more=" << has_more;
    const int task_id = tracker_.PostTask(
        base::ThreadTaskRunnerHandle::Get().get(), FROM_HERE,
        base::BindOnce(callback, read_len, has_more, base::File::FILE_OK));
    task_ids.push_back(task_id);
    remaining -= read_len;
    upto += read_len;
    // LOG(ERROR) << "SmbProvidedFileSystem::ReadFile end loop remaining="
    //           << remaining << " upto=" << upto;
  } while (remaining > 0 && upto < kFakeFileSize);

  // LOG(ERROR) << "SmbProvidedFileSystem::ReadFile DONE";
  return base::Bind(&SmbProvidedFileSystem::AbortMany,
                    weak_ptr_factory_.GetWeakPtr(), task_ids);
}

AbortCallback SmbProvidedFileSystem::CreateDirectory(
    const base::FilePath& directory_path,
    bool recursive,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

AbortCallback SmbProvidedFileSystem::DeleteEntry(
    const base::FilePath& entry_path,
    bool recursive,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

AbortCallback SmbProvidedFileSystem::CreateFile(
    const base::FilePath& file_path,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  const base::File::Error result = file_path.AsUTF8Unsafe() != kFakeFilePath
                                       ? base::File::FILE_ERROR_EXISTS
                                       : base::File::FILE_OK;

  return PostAbortableTask(base::Bind(callback, result));
}

AbortCallback SmbProvidedFileSystem::CopyEntry(
    const base::FilePath& source_path,
    const base::FilePath& target_path,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

AbortCallback SmbProvidedFileSystem::MoveEntry(
    const base::FilePath& source_path,
    const base::FilePath& target_path,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

AbortCallback SmbProvidedFileSystem::Truncate(
    const base::FilePath& file_path,
    int64_t length,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

AbortCallback SmbProvidedFileSystem::WriteFile(
    int file_handle,
    net::IOBuffer* buffer,
    int64_t offset,
    int length,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // LOG(ERROR) << "SmbProvidedFileSystem::WriteFile ofst=" << offset
  //            << " len=" << length;

  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

AbortCallback SmbProvidedFileSystem::AddWatcher(
    const GURL& origin,
    const base::FilePath& entry_watcher,
    bool recursive,
    bool persistent,
    const storage::AsyncFileUtil::StatusCallback& callback,
    const storage::WatcherManager::NotificationCallback&
        notification_callback) {
  // TODO(mtomasz): Implement it once needed.
  return PostAbortableTask(base::Bind(callback, base::File::FILE_OK));
}

void SmbProvidedFileSystem::RemoveWatcher(
    const GURL& origin,
    const base::FilePath& entry_path,
    bool recursive,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  // TODO(mtomasz): Implement it once needed.
  callback.Run(base::File::FILE_OK);
}

const ProvidedFileSystemInfo& SmbProvidedFileSystem::GetFileSystemInfo() const {
  return file_system_info_;
}

RequestManager* SmbProvidedFileSystem::GetRequestManager() {
  NOTREACHED();
  return NULL;
}

Watchers* SmbProvidedFileSystem::GetWatchers() {
  return &watchers_;
}

const OpenedFiles& SmbProvidedFileSystem::GetOpenedFiles() const {
  return opened_files_;
}

void SmbProvidedFileSystem::AddObserver(ProvidedFileSystemObserver* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void SmbProvidedFileSystem::RemoveObserver(
    ProvidedFileSystemObserver* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void SmbProvidedFileSystem::Notify(
    const base::FilePath& entry_path,
    bool recursive,
    storage::WatcherManager::ChangeType change_type,
    std::unique_ptr<ProvidedFileSystemObserver::Changes> changes,
    const std::string& tag,
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTREACHED();
  callback.Run(base::File::FILE_ERROR_SECURITY);
}

void SmbProvidedFileSystem::Configure(
    const storage::AsyncFileUtil::StatusCallback& callback) {
  NOTREACHED();
  callback.Run(base::File::FILE_ERROR_SECURITY);
}

std::unique_ptr<ProvidedFileSystemInterface> SmbProvidedFileSystem::Create(
    Profile* profile,
    const ProvidedFileSystemInfo& file_system_info) {
  return base::MakeUnique<SmbProvidedFileSystem>(file_system_info);
}

base::WeakPtr<ProvidedFileSystemInterface> SmbProvidedFileSystem::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

AbortCallback SmbProvidedFileSystem::PostAbortableTask(
    const base::Closure& callback) {
  const int task_id = tracker_.PostTask(
      base::ThreadTaskRunnerHandle::Get().get(), FROM_HERE, callback);
  return base::Bind(&SmbProvidedFileSystem::Abort,
                    weak_ptr_factory_.GetWeakPtr(), task_id);
}

void SmbProvidedFileSystem::Abort(int task_id) {
  tracker_.TryCancel(task_id);
}

void SmbProvidedFileSystem::AbortMany(const std::vector<int>& task_ids) {
  for (size_t i = 0; i < task_ids.size(); ++i) {
    tracker_.TryCancel(task_ids[i]);
  }
}

}  // namespace file_system_provider
}  // namespace chromeos
