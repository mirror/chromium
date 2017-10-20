// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/cdm_storage_impl.h"

#include <map>
#include <memory>
#include <tuple>
#include <utility>

#include "base/callback.h"
#include "base/logging.h"
#include "base/task_scheduler/post_task.h"
#include "content/browser/media/cdm_file_impl.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "ppapi/shared_impl/ppapi_constants.h"
#include "storage/browser/fileapi/async_file_util.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation_context.h"
#include "storage/browser/fileapi/isolated_context.h"
#include "storage/browser/quota/quota_manager.h"
#include "url/origin.h"

// Currently this uses the PluginPrivateFileSystem as the previous CDMs ran
// as pepper plugins and we need to be able to access any existing files.
// TODO(jrummell): Switch to using a separate file system once CDMs no
// longer run as pepper plugins.

namespace content {

namespace {

// File map shared by all CdmStorageImpl objects to prevent read/write race.
// A lock must be acquired before opening a file to ensure that the file is not
// currently in use. Once the file is opened the file map must be updated to
// include the callback used to close the file. The lock must be held until
// the file is closed.
class FileLockMap {
 public:
  using Key = CdmStorageImpl::FileLockKey;

  FileLockMap() = default;
  ~FileLockMap() = default;

  // Acquire a lock on the file represented by |key|. Returns true if |key|
  // is not currently in use, false otherwise.
  bool AcquireFileLock(const Key& key) {
    DVLOG(3) << __func__ << " file: " << key.file_name;
    DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

    // Add a new entry. If |key| already has an entry, emplace() tells so
    // with the second piece of the returned value and does not modify
    // the original.
    return file_lock_map_.emplace(key).second;
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

// Helper function to call FileLockMap::ReleaseFileLock().
void ReleaseFileLock(const CdmStorageImpl::FileLockKey& key) {
  GetFileLockMap()->ReleaseFileLock(key);
}

}  // namespace

CdmStorageImpl::FileLockKey::FileLockKey(const std::string& cdm_file_system_id,
                                         const url::Origin& origin,
                                         const std::string& file_name)
    : cdm_file_system_id(cdm_file_system_id),
      origin(origin),
      file_name(file_name) {}

bool CdmStorageImpl::FileLockKey::operator<(const FileLockKey& other) const {
  return std::tie(cdm_file_system_id, origin, file_name) <
         std::tie(other.cdm_file_system_id, other.origin, other.file_name);
}

// static
void CdmStorageImpl::Create(RenderFrameHost* render_frame_host,
                            const std::string& cdm_file_system_id,
                            media::mojom::CdmStorageRequest request) {
  DVLOG(3) << __func__;
  DCHECK(!render_frame_host->GetLastCommittedOrigin().unique())
      << "Invalid origin specified for CdmStorageImpl::Create";

  // Take a reference to the FileSystemContext.
  scoped_refptr<storage::FileSystemContext> file_system_context;
  StoragePartition* storage_partition =
      render_frame_host->GetProcess()->GetStoragePartition();
  if (storage_partition)
    file_system_context = storage_partition->GetFileSystemContext();

  // The created object is bound to (and owned by) |request|.
  new CdmStorageImpl(render_frame_host, cdm_file_system_id,
                     std::move(file_system_context), std::move(request));
}

// static
bool CdmStorageImpl::IsValidCdmFileSystemId(
    const std::string& cdm_file_system_id) {
  // To be compatible with PepperFileSystemBrowserHost::GeneratePluginId(),
  // |cdm_file_system_id| must contain only letters (A-Za-z), digits(0-9),
  // or "._-".
  for (const auto& ch : cdm_file_system_id) {
    if (!base::IsAsciiAlpha(ch) && !base::IsAsciiDigit(ch) && ch != '.' &&
        ch != '_' && ch != '-') {
      return false;
    }
  }

  // Also ensure that |cdm_file_system_id| contains at least 1 character.
  return !cdm_file_system_id.empty();
}

CdmStorageImpl::CdmStorageImpl(
    RenderFrameHost* render_frame_host,
    const std::string& cdm_file_system_id,
    scoped_refptr<storage::FileSystemContext> file_system_context,
    media::mojom::CdmStorageRequest request)
    : FrameServiceBase(render_frame_host, std::move(request)),
      cdm_file_system_id_(cdm_file_system_id),
      file_system_context_(std::move(file_system_context)),
      child_process_id_(render_frame_host->GetProcess()->GetID()),
      weak_factory_(this) {}

CdmStorageImpl::~CdmStorageImpl() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // If any file locks were taken in Open() and never made it to OnFileOpened(),
  // release them now as OnFileOpened() will never run.
  for (const auto& file_lock_key : pending_open_)
    ReleaseFileLock(file_lock_key);
}

void CdmStorageImpl::Open(const std::string& file_name, OpenCallback callback) {
  DVLOG(3) << __func__ << " file: " << file_name;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!IsValidCdmFileSystemId(cdm_file_system_id_)) {
    DVLOG(1) << "CdmStorageImpl not initialized properly.";
    std::move(callback).Run(Status::kFailure, base::File(), nullptr);
    return;
  }

  if (file_name.empty()) {
    DVLOG(1) << "No file specified.";
    std::move(callback).Run(Status::kFailure, base::File(), nullptr);
    return;
  }

  FileLockKey file_lock_key(cdm_file_system_id_, origin(), file_name);
  if (!GetFileLockMap()->AcquireFileLock(file_lock_key)) {
    DVLOG(1) << "File " << file_name << " is already in use.";
    std::move(callback).Run(Status::kInUse, base::File(), nullptr);
    return;
  }

  // In case this object gets destroyed before everything completes, keep
  // track of |file_lock_key| so that it can be released if the file is
  // never successfully opened.
  pending_open_.insert(file_lock_key);

  OpenFileSystem(base::Bind(&CdmStorageImpl::OpenFileIfFileSystemOpened,
                            weak_factory_.GetWeakPtr(), file_lock_key,
                            base::Passed(&callback)));
}

void CdmStorageImpl::OpenFileIfFileSystemOpened(
    const FileLockKey& file_lock_key,
    OpenCallback callback,
    base::File::Error error) {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (error != base::File::FILE_OK) {
    pending_open_.erase(file_lock_key);
    ReleaseFileLock(file_lock_key);
    std::move(callback).Run(Status::kFailure, base::File(), nullptr);
    return;
  }

  OpenFile(file_lock_key.file_name,
           base::File::FLAG_OPEN_ALWAYS | base::File::FLAG_READ,
           base::Bind(&CdmStorageImpl::OnFileOpened, weak_factory_.GetWeakPtr(),
                      file_lock_key, base::Passed(&callback)));
}

void CdmStorageImpl::OnFileOpened(const FileLockKey& file_lock_key,
                                  OpenCallback callback,
                                  base::File file,
                                  const base::Closure& on_close_callback) {
  // |on_close_callback| should be called after the |file| is closed in the
  // child process. See AsyncFileUtil for details.
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // |file_lock_key| will either be used by CdmFileImpl on a successful
  // open or released if |file| is not valid, so the open is no longer pending.
  pending_open_.erase(file_lock_key);

  if (!file.IsValid()) {
    DLOG(WARNING) << "Unable to open file " << file_lock_key.file_name
                  << ", error: "
                  << base::File::ErrorToString(file.error_details());
    ReleaseFileLock(file_lock_key);
    std::move(callback).Run(Status::kFailure, base::File(), nullptr);
    return;
  }

  // When the connection to |cdm_file| is closed, |on_close_callback|
  // will be run.
  media::mojom::CdmFileAssociatedPtrInfo cdm_file;
  cdm_file_bindings_.AddBinding(
      std::make_unique<CdmFileImpl>(
          file_lock_key.file_name, this, std::move(on_close_callback),
          base::BindOnce(&ReleaseFileLock, file_lock_key)),
      mojo::MakeRequest(&cdm_file));
  std::move(callback).Run(Status::kSuccess, std::move(file),
                          std::move(cdm_file));
}

void CdmStorageImpl::OpenFileSystem(FileSystemOpenedCallback callback) {
  DVLOG(3) << __func__;

  // Only open the file system once.
  if (file_system_state_ == FileSystemState::kOpened) {
    std::move(callback).Run(file_system_opened_result_);
    return;
  }

  // Save |callback| for when the file system is open. If it is already in
  // progress, nothing more to do until the existing
  // OpenPluginPrivateFileSystem() call completes.
  pending_open_filesystem_callbacks_.push_back(std::move(callback));
  if (file_system_state_ == FileSystemState::kOpening)
    return;

  DCHECK_EQ(FileSystemState::kUnopened, file_system_state_);
  file_system_state_ = FileSystemState::kOpening;

  std::string fsid =
      storage::IsolatedContext::GetInstance()->RegisterFileSystemForVirtualPath(
          storage::kFileSystemTypePluginPrivate, ppapi::kPluginPrivateRootName,
          base::FilePath());
  if (!storage::ValidateIsolatedFileSystemId(fsid)) {
    DVLOG(1) << "Invalid file system ID.";
    std::move(callback).Run(base::File::FILE_ERROR_NOT_FOUND);
    return;
  }

  // Grant full access of isolated file system to child process.
  ChildProcessSecurityPolicy::GetInstance()->GrantCreateReadWriteFileSystem(
      child_process_id_, fsid);

  // Keep track of the URI for this instance of the PluginPrivateFileSystem.
  file_system_root_uri_ = storage::GetIsolatedFileSystemRootURIString(
      origin().GetURL(), fsid, ppapi::kPluginPrivateRootName);

  file_system_context_->OpenPluginPrivateFileSystem(
      origin().GetURL(), storage::kFileSystemTypePluginPrivate, fsid,
      cdm_file_system_id_, storage::OPEN_FILE_SYSTEM_CREATE_IF_NONEXISTENT,
      base::Bind(&CdmStorageImpl::OnFileSystemOpened,
                 weak_factory_.GetWeakPtr()));
}

void CdmStorageImpl::OnFileSystemOpened(base::File::Error error) {
  file_system_state_ = FileSystemState::kOpened;
  file_system_opened_result_ = error;

  // Now run any saved callbacks with the result received.
  std::vector<FileSystemOpenedCallback> pending_callbacks;
  pending_callbacks.swap(pending_open_filesystem_callbacks_);
  for (size_t i = 0; i < pending_callbacks.size(); ++i)
    std::move(pending_callbacks[i]).Run(file_system_opened_result_);
}

void CdmStorageImpl::OpenFile(const std::string& file_name,
                              int flags,
                              FileOpenedCallback callback) {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  storage::FileSystemURL file_url =
      file_system_context_->CrackURL(GURL(file_system_root_uri_ + file_name));
  storage::AsyncFileUtil* file_util = file_system_context_->GetAsyncFileUtil(
      storage::kFileSystemTypePluginPrivate);
  auto operation_context =
      std::make_unique<storage::FileSystemOperationContext>(
          file_system_context_.get());
  operation_context->set_allowed_bytes_growth(storage::QuotaManager::kNoLimit);
  DVLOG(1) << "Opening " << file_url.DebugString();
  file_util->CreateOrOpen(std::move(operation_context), file_url, flags,
                          callback);
}

void CdmStorageImpl::RenameFile(const std::string& src_file_name,
                                const std::string& dest_file_name,
                                FileRenamedCallback callback) {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  storage::FileSystemURL src_file_url = file_system_context_->CrackURL(
      GURL(file_system_root_uri_ + src_file_name));
  storage::FileSystemURL dest_file_url = file_system_context_->CrackURL(
      GURL(file_system_root_uri_ + dest_file_name));
  storage::AsyncFileUtil* file_util = file_system_context_->GetAsyncFileUtil(
      storage::kFileSystemTypePluginPrivate);
  auto operation_context =
      std::make_unique<storage::FileSystemOperationContext>(
          file_system_context_.get());
  DVLOG(1) << "Renaming " << src_file_url.DebugString() << " to "
           << dest_file_url.DebugString();
  file_util->MoveFileLocal(std::move(operation_context), src_file_url,
                           dest_file_url,
                           storage::FileSystemOperation::OPTION_NONE, callback);
}

}  // namespace content
