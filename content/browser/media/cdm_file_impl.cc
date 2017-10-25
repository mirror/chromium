// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/cdm_file_impl.h"

#include <set>
#include <utility>

#include "base/callback.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation_context.h"
#include "storage/browser/fileapi/file_system_url.h"
#include "storage/browser/quota/quota_manager.h"
#include "storage/common/fileapi/file_system_types.h"

namespace content {

namespace {

// The CDM interface has a restriction that file names can not begin with _,
// so use it to prefix temporary files.
const char kTemporaryFilePrefix[] = "_";

std::string GetTempFileName(const std::string& file_name) {
  DCHECK(!base::StartsWith(file_name, kTemporaryFilePrefix,
                           base::CompareCase::SENSITIVE));
  return kTemporaryFilePrefix + file_name;
}

// File map shared by all CdmStorageImpl objects to prevent read/write race.
// A lock must be acquired before opening a file to ensure that the file is not
// currently in use. Once the file is opened the file map must be updated to
// include the callback used to close the file. The lock must be held until
// the file is closed.
class FileLockMap {
 public:
  using Key = CdmFileImpl::FileLockKey;

  FileLockMap() = default;
  ~FileLockMap() = default;

  // Acquire a lock on the file represented by |key|. Returns true if |key|
  // is not currently in use, false otherwise.
  bool AcquireFileLock(const Key& key) {
    DVLOG(3) << __func__ << " file: " << key.file_name;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    // Add a new entry. If |key| already has an entry, insert() tells so
    // with the second piece of the returned value and does not modify
    // the original.
    return file_lock_map_.insert(key).second;
  }

  // Tests whether a lock is held on |key| or not. Returns true if |key|
  // is currently locked, false otherwise.
  bool IsFileLockHeld(const Key& key) {
    DVLOG(3) << __func__ << " file: " << key.file_name;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    // Lock is held if there is an entry for |key|.
    return file_lock_map_.count(key) > 0;
  }

  // Release the lock held on the file represented by |key|. If
  // |on_close_callback| has been set, run it before releasing the lock.
  void ReleaseFileLock(const Key& key) {
    DVLOG(3) << __func__ << " file: " << key.file_name;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    auto entry = file_lock_map_.find(key);
    if (entry == file_lock_map_.end()) {
      NOTREACHED() << "Unable to release lock on file " << key.file_name;
      return;
    }

    file_lock_map_.erase(entry);
  }

 private:
  // Note that this map is never deleted. As entries are removed when a file
  // is closed, it should never get too large.
  std::set<Key> file_lock_map_;

  THREAD_CHECKER(thread_checker_);
  DISALLOW_COPY_AND_ASSIGN(FileLockMap);
};

FileLockMap* GetFileLockMap() {
  static auto* file_lock_map = new FileLockMap();
  return file_lock_map;
}

}  // namespace

CdmFileImpl::FileLockKey::FileLockKey(const std::string& file_system_id,
                                      const url::Origin& origin,
                                      const std::string& file_name)
    : file_system_id(file_system_id), origin(origin), file_name(file_name) {}

bool CdmFileImpl::FileLockKey::operator<(const FileLockKey& other) const {
  return std::tie(file_system_id, origin, file_name) <
         std::tie(other.file_system_id, other.origin, other.file_name);
}

CdmFileImpl::CdmFileImpl(
    const std::string& file_name,
    const url::Origin& origin,
    const std::string& file_system_id,
    scoped_refptr<storage::FileSystemContext> file_system_context,
    const std::string& file_system_root_uri)
    : file_name_(file_name),
      temp_file_name_(GetTempFileName(file_name_)),
      origin_(origin),
      file_system_id_(file_system_id),
      file_system_context_(file_system_context),
      file_system_root_uri_(file_system_root_uri),
      weak_factory_(this) {
  DVLOG(3) << __func__ << " " << file_name_;
}

CdmFileImpl::~CdmFileImpl() {
  DVLOG(3) << __func__ << " " << file_name_;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (state_ == FileState::kOpenForReadAndWrite) {
    // Temporary file is open, so close and release it.
    if (temporary_file_on_close_callback_)
      std::move(temporary_file_on_close_callback_).Run();
    ReleaseFileLock(temp_file_name_);
  }
  if (state_ != FileState::kNone) {
    // Original file is open, so close and release it.
    if (on_close_callback_)
      std::move(on_close_callback_).Run();
    ReleaseFileLock(file_name_);
  }
}

void CdmFileImpl::Initialize(OpenFileCallback callback) {
  DVLOG(3) << __func__ << " file: " << file_name_;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // Grab the lock on |file_name_|. The lock will be held until this object is
  // destructed.
  if (!AcquireFileLock(file_name_)) {
    DVLOG(3) << "File " << file_name_ << " is already in use.";
    std::move(callback).Run(base::File(base::File::FILE_ERROR_IN_USE));
    return;
  }

  // We have the lock on |file_name_|. Now open the file for reading. Since
  // we don't know if this file exists or not, provide FLAG_OPEN_ALWAYS to
  // create the file if it doesn't exist.
  state_ = FileState::kOpenForRead;
  OpenFile(file_name_, base::File::FLAG_OPEN_ALWAYS | base::File::FLAG_READ,
           base::Bind(&CdmFileImpl::OnFileOpenedForRead,
                      weak_factory_.GetWeakPtr(), base::Passed(&callback)));
}

void CdmFileImpl::OpenFile(const std::string& file_name,
                           uint32_t file_flags,
                           CreateOrOpenCallback callback) {
  DVLOG(3) << __func__ << " file: " << file_name << ", flags: " << file_flags;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_NE(FileState::kNone, state_);
  DCHECK(IsFileLockHeld(file_name));

  storage::FileSystemURL file_url = CreateFileSystemURL(file_name);
  storage::AsyncFileUtil* file_util = file_system_context_->GetAsyncFileUtil(
      storage::kFileSystemTypePluginPrivate);
  auto operation_context =
      std::make_unique<storage::FileSystemOperationContext>(
          file_system_context_.get());
  operation_context->set_allowed_bytes_growth(storage::QuotaManager::kNoLimit);
  DVLOG(3) << "Opening " << file_url.DebugString();
  file_util->CreateOrOpen(std::move(operation_context), file_url, file_flags,
                          std::move(callback));
}

void CdmFileImpl::OnFileOpenedForRead(OpenFileCallback callback,
                                      base::File file,
                                      const base::Closure& on_close_callback) {
  DVLOG(3) << __func__ << " file: " << file_name_;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(FileState::kOpenForRead, state_);

  if (!file.IsValid()) {
    // File is invalid. Note that the lock on |file_name_| is kept until this
    // object is destructed.
    DLOG(WARNING) << "Unable to open file " << file_name_ << ", error: "
                  << base::File::ErrorToString(file.error_details());
    std::move(callback).Run(std::move(file));
    return;
  }

  // When the file is closed, |on_close_callback| will be run.
  on_close_callback_ = std::move(on_close_callback);
  std::move(callback).Run(std::move(file));
}

void CdmFileImpl::OpenTempFileForWriting(
    OpenTempFileForWritingCallback callback) {
  DVLOG(3) << __func__ << " " << file_name_;

  // Grab a lock on the temporary file. The lock will be held until this
  // new file is renamed in RenameTempFileAndReopen() (or this object is
  // destructed).
  if (!AcquireFileLock(temp_file_name_)) {
    DVLOG(3) << "File " << temp_file_name_ << " is already in use.";
    std::move(callback).Run(base::File(base::File::FILE_ERROR_IN_USE));
    return;
  }

  // We now have locks on both |file_name_| and |temp_file_name_|. Open the
  // temporary file for writing. Specifying FLAG_CREATE_ALWAYS which will
  // overwrite any existing file.
  state_ = FileState::kOpenForReadAndWrite;
  OpenFile(temp_file_name_,
           base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE,
           base::Bind(&CdmFileImpl::OnTempFileOpenedForWrite,
                      weak_factory_.GetWeakPtr(), base::Passed(&callback)));
}

void CdmFileImpl::OnTempFileOpenedForWrite(
    OpenTempFileForWritingCallback callback,
    base::File file,
    const base::Closure& on_close_callback) {
  DVLOG(3) << __func__ << " file: " << file_name_;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(FileState::kOpenForReadAndWrite, state_);

  if (!file.IsValid()) {
    DLOG(WARNING) << "Unable to open file " << temp_file_name_ << ", error: "
                  << base::File::ErrorToString(file.error_details());
    state_ = FileState::kOpenForRead;
    ReleaseFileLock(temp_file_name_);
    std::move(callback).Run(std::move(file));
    return;
  }

  temporary_file_on_close_callback_ = std::move(on_close_callback);
  std::move(callback).Run(std::move(file));
}

void CdmFileImpl::RenameTempFileAndReopen(
    RenameTempFileAndReopenCallback callback) {
  DVLOG(3) << __func__ << " " << temp_file_name_ << " to " << file_name_;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(FileState::kOpenForReadAndWrite, state_);
  DCHECK(IsFileLockHeld(file_name_));
  DCHECK(IsFileLockHeld(temp_file_name_));

  if (on_close_callback_)
    std::move(on_close_callback_).Run();
  if (temporary_file_on_close_callback_)
    std::move(temporary_file_on_close_callback_).Run();

  storage::FileSystemURL src_file_url = CreateFileSystemURL(temp_file_name_);
  storage::FileSystemURL dest_file_url = CreateFileSystemURL(file_name_);
  storage::AsyncFileUtil* file_util = file_system_context_->GetAsyncFileUtil(
      storage::kFileSystemTypePluginPrivate);
  auto operation_context =
      std::make_unique<storage::FileSystemOperationContext>(
          file_system_context_.get());
  DVLOG(3) << "Renaming " << src_file_url.DebugString() << " to "
           << dest_file_url.DebugString();
  file_util->MoveFileLocal(
      std::move(operation_context), src_file_url, dest_file_url,
      storage::FileSystemOperation::OPTION_NONE,
      base::Bind(&CdmFileImpl::OnTempFileRenamed, weak_factory_.GetWeakPtr(),
                 base::Passed(&callback)));
}

void CdmFileImpl::OnTempFileRenamed(RenameTempFileAndReopenCallback callback,
                                    base::File::Error move_result) {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK_EQ(FileState::kOpenForReadAndWrite, state_);

  // Temporary file has been renamed, so we can release the lock on it.
  ReleaseFileLock(temp_file_name_);
  state_ = FileState::kOpenForRead;

  // Was the rename successful?
  if (move_result != base::File::FILE_OK) {
    std::move(callback).Run(base::File(move_result));
    return;
  }

  // Reopen the original file for reading. Specifying FLAG_OPEN as the file
  // has to exist or something's wrong.
  OpenFile(file_name_, base::File::FLAG_OPEN | base::File::FLAG_READ,
           base::Bind(&CdmFileImpl::OnFileOpenedForRead,
                      weak_factory_.GetWeakPtr(), base::Passed(&callback)));
}

storage::FileSystemURL CdmFileImpl::CreateFileSystemURL(
    const std::string& file_name) {
  return file_system_context_->CrackURL(
      GURL(file_system_root_uri_ + file_name));
}

bool CdmFileImpl::AcquireFileLock(const std::string& file_name) {
  FileLockKey file_lock_key(file_system_id_, origin_, file_name);
  return GetFileLockMap()->AcquireFileLock(file_lock_key);
}

bool CdmFileImpl::IsFileLockHeld(const std::string& file_name) {
  FileLockKey file_lock_key(file_system_id_, origin_, file_name);
  return GetFileLockMap()->IsFileLockHeld(file_lock_key);
}

void CdmFileImpl::ReleaseFileLock(const std::string& file_name) {
  FileLockKey file_lock_key(file_system_id_, origin_, file_name);
  GetFileLockMap()->ReleaseFileLock(file_lock_key);
}

}  // namespace content
