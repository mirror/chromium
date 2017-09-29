// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/cdm_storage_impl.h"

#include <map>
#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "media/base/key_system_names.h"
#include "media/cdm/cdm_paths.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "ppapi/shared_impl/ppapi_constants.h"
#include "storage/browser/fileapi/async_file_util.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation_context.h"
#include "storage/browser/fileapi/isolated_context.h"
#include "storage/browser/quota/quota_manager.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"

// Currently this uses the PepperPlugInFileSystem as the previous CDMs ran
// as pepper plugins and we need to be able to access any existing files.
// TODO(jrummell): Switch to using a separate filesystem once CDMs no
// longer run as pepper plugins.

namespace {

// A file name should not:
//    (1) be empty,
//    (2) start with '_',
// or (3) contain any path separators.
bool IsValidFileName(const std::string& file_name) {
  return (!file_name.empty() && file_name[0] != '_' &&
          file_name.find('/') == std::string::npos &&
          file_name.find('\\') == std::string::npos);
}

// Generate the file system ID used by the plugin specified with |mime_type|.
// This is similar to PepperFileSystemBrowserHost::GeneratePluginId(), but
// with fewer checks since |mime_type| is constant strings.
// TODO(jrummell): Find a common place to put this functionality.
std::string GenerateFileSystemId(const std::string& mime_type) {
  // As '/' is not allowed in file names, replace any with '_'.
  std::string result = mime_type;
  std::replace(result.begin(), result.end(), '/', '_');
  DCHECK(IsValidFileName(result));
  return result;
}

// File map shared by all CdmStorageImpl objects to prevent read/write race.
// A lock must be acquired before opening a file to ensure that the file is not
// currently in use. Once the file is opened the file map must be updated to
// include the callback used to close the file. The lock is held until the file
// is closed (determined by the connection with the client being broken).
// Notes:
// - This map is never deleted. As entries are removed when the file is
//   closed, this map should never get too large.
using FileLockMap = std::map<CdmStorageImpl::FileLockKey, base::Closure>;

FileLockMap* GetFileMap() {
  static auto* file_map = new FileLockMap();
  return file_map;
}

// Use a base::Lock() to ensure that only one instance accesses |file_map|
// at a time.
base::Lock* GetFileMapLock() {
  static auto* lock = new base::Lock();
  return lock;
}

// Acquire a lock on the file represented by |key|. Returns true if |key|
// is not currently in use, false otherwise.
bool AcquireFileLock(const CdmStorageImpl::FileLockKey& key) {
  DVLOG(3) << __func__ << " file: " << std::get<2>(key);

  base::AutoLock auto_lock(*GetFileMapLock());
  // Make sure there isn't an existing entry for this file.
  if (GetFileMap()->count(key))
    return false;

  // Add a new entry.
  return GetFileMap()->emplace(key, base::Closure()).second;
}

// Update the entry for the file represented by |key| so that
// |on_close_callback| will be called when the lock is released.
void SetOnCloseCallback(const CdmStorageImpl::FileLockKey& key,
                        base::Closure on_close_callback) {
  DVLOG(3) << __func__ << " file: " << std::get<2>(key);

  base::AutoLock auto_lock(*GetFileMapLock());
  DCHECK(!GetFileMap()->empty());
  auto entry = GetFileMap()->find(key);
  DCHECK(entry != GetFileMap()->end());
  entry->second = std::move(on_close_callback);
}

void ReleaseFileLock(const CdmStorageImpl::FileLockKey& key) {
  DVLOG(3) << __func__ << " file: " << std::get<2>(key);

  base::AutoLock auto_lock(*GetFileMapLock());
  auto entry = GetFileMap()->find(key);
  if (entry == GetFileMap()->end()) {
    NOTREACHED() << "Unable to relase lock on file " << std::get<2>(key);
    return;
  }

  if (entry->second)
    entry->second.Run();
  GetFileMap()->erase(entry);
}

// mojom::CdmStorage::Open() returns a mojom::CdmFileReleaser reference to keep
// track of the file being used. This object is created when the file is being
// passed to the client. When the client is done using the file, the connection
// should be broken and this will release the lock held on the file.
class CdmFileReleaserImpl final : public media::mojom::CdmFileReleaser {
 public:
  explicit CdmFileReleaserImpl(const CdmStorageImpl::FileLockKey& key)
      : key_(key) {
    DVLOG(1) << __func__;
  }

  ~CdmFileReleaserImpl() override {
    DVLOG(1) << __func__;
    ReleaseFileLock(key_);
  }

 private:
  CdmStorageImpl::FileLockKey key_;

  DISALLOW_COPY_AND_ASSIGN(CdmFileReleaserImpl);
};

}  // namespace

// static
void CdmStorageImpl::Create(content::RenderFrameHost* render_frame_host,
                            media::mojom::CdmStorageRequest request) {
  DVLOG(3) << __func__;

  // Take a reference to the FileSystemContext, if it exists.
  scoped_refptr<storage::FileSystemContext> filesystem_context;
  content::StoragePartition* storage_partition =
      render_frame_host->GetProcess()->GetStoragePartition();
  if (storage_partition)
    filesystem_context = storage_partition->GetFileSystemContext();
  DCHECK(!render_frame_host->GetLastCommittedOrigin().unique())
      << "Invalid origin specified for CdmStorageImpl::Create";

  // The created object is bound to (and owned by) |request|.
  new CdmStorageImpl(render_frame_host->GetProcess()->GetID(),
                     render_frame_host->GetLastCommittedOrigin(),
                     std::move(filesystem_context), std::move(request));
}

CdmStorageImpl::CdmStorageImpl(
    int child_process_id,
    const url::Origin& origin,
    scoped_refptr<storage::FileSystemContext> filesystem_context,
    media::mojom::CdmStorageRequest request)
    : child_process_id_(child_process_id),
      origin_(origin),
      binding_(this, std::move(request)),
      filesystem_context_(std::move(filesystem_context)),
      weak_factory_(this) {
  binding_.set_connection_error_handler(
      base::BindOnce(&CdmStorageImpl::Close, weak_factory_.GetWeakPtr()));
}

CdmStorageImpl::~CdmStorageImpl() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // If any file locks were taken in Open() and never made it to OnFileOpened(),
  // release them now as OnFileOpened() will never run.
  weak_factory_.InvalidateWeakPtrs();
  for (const auto& file_lock_key : pending_open_)
    ReleaseFileLock(file_lock_key);
}

void CdmStorageImpl::Initialize(const std::string& key_system) {
  DVLOG(3) << __func__ << " key system: " << key_system;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // TODO(crbug.com/510604): Rather than use |key_system|, use CdmRegistry
  // once fully implemented to look up the CdmType, and use that to determine
  // the file system ID.
  if (key_system == kWidevineKeySystem) {
    plugin_id_ = GenerateFileSystemId(kWidevineCdmPluginMimeType);
    return;
  }

  if (media::IsExternalClearKey(key_system)) {
    plugin_id_ = GenerateFileSystemId(media::kClearKeyCdmPepperMimeType);
    return;
  }

  DLOG(WARNING) << "Unrecognized key system " << key_system;
}

void CdmStorageImpl::Open(const std::string& file_name, OpenCallback callback) {
  DVLOG(3) << __func__ << " file: " << file_name;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (plugin_id_.empty()) {
    DVLOG(1) << "CdmStorageImpl not initialized properly.";
    std::move(callback).Run(Status::FAILURE, base::File(), nullptr);
    return;
  }

  if (!filesystem_context_) {
    DVLOG(1) << "Unable to get FileSystemContext.";
    std::move(callback).Run(Status::FAILURE, base::File(), nullptr);
    return;
  }

  if (!IsValidFileName(file_name)) {
    DVLOG(1) << "Invalid file name " << file_name;
    std::move(callback).Run(Status::FAILURE, base::File(), nullptr);
    return;
  }

  FileLockKey file_lock_key = std::make_tuple(plugin_id_, origin_, file_name);
  if (!AcquireFileLock(file_lock_key)) {
    DVLOG(1) << "File " << file_name << " is already in use.";
    std::move(callback).Run(Status::IN_USE, base::File(), nullptr);
    return;
  }

  std::string fsid =
      storage::IsolatedContext::GetInstance()->RegisterFileSystemForVirtualPath(
          storage::kFileSystemTypePluginPrivate, ppapi::kPluginPrivateRootName,
          base::FilePath());
  if (!storage::ValidateIsolatedFileSystemId(fsid)) {
    DVLOG(1) << "Invalid file system ID.";
    ReleaseFileLock(file_lock_key);
    std::move(callback).Run(Status::FAILURE, base::File(), nullptr);
    return;
  }

  // In case this object gets destroyed before OpenPluginPrivateFileSystem()
  // completes, keep track of |file_lock_key| so that it can be released if
  // OnFileSystemOpened() or OnFileOpened() never get called.
  pending_open_.insert(file_lock_key);

  // Grant full access of isolated filesystem to child process.
  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  policy->GrantCreateReadWriteFileSystem(child_process_id_, fsid);

  filesystem_context_->OpenPluginPrivateFileSystem(
      origin_.GetURL(), storage::kFileSystemTypePluginPrivate, fsid, plugin_id_,
      storage::OPEN_FILE_SYSTEM_CREATE_IF_NONEXISTENT,
      base::Bind(&CdmStorageImpl::OnFileSystemOpened,
                 weak_factory_.GetWeakPtr(), file_lock_key, fsid,
                 base::Passed(&callback)));
}

void CdmStorageImpl::OnFileSystemOpened(const FileLockKey& file_lock_key,
                                        const std::string& fsid,
                                        OpenCallback callback,
                                        base::File::Error error) {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (error != base::File::FILE_OK) {
    DVLOG(1) << "Unable to access the file system.";
    pending_open_.erase(file_lock_key);
    ReleaseFileLock(file_lock_key);
    std::move(callback).Run(Status::FAILURE, base::File(), nullptr);
    return;
  }

  std::string root = storage::GetIsolatedFileSystemRootURIString(
                         GURL(origin_.Serialize()), fsid, "pluginprivate")
                         .append(std::get<2>(file_lock_key));
  storage::FileSystemURL file_url = filesystem_context_->CrackURL(GURL(root));
  storage::AsyncFileUtil* file_util = filesystem_context_->GetAsyncFileUtil(
      storage::kFileSystemTypePluginPrivate);
  std::unique_ptr<storage::FileSystemOperationContext> operation_context =
      std::make_unique<storage::FileSystemOperationContext>(
          filesystem_context_.get());
  operation_context->set_allowed_bytes_growth(storage::QuotaManager::kNoLimit);
  int flags = base::File::FLAG_OPEN_ALWAYS | base::File::FLAG_READ |
              base::File::FLAG_WRITE;
  DVLOG(1) << "Opening " << file_url.DebugString();
  file_util->CreateOrOpen(
      std::move(operation_context), file_url, flags,
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

  // |file_lock_key| will either be used by CdmFileReleaserImpl on a successful
  // open or released if |file| is not valid, so the open is no longer pending.
  pending_open_.erase(file_lock_key);

  if (!file.IsValid()) {
    DLOG(WARNING) << "Unable to open file " << std::get<2>(file_lock_key)
                  << ", error: "
                  << base::File::ErrorToString(file.error_details());
    ReleaseFileLock(file_lock_key);
    std::move(callback).Run(Status::FAILURE, base::File(), nullptr);
    return;
  }

  SetOnCloseCallback(file_lock_key, std::move(on_close_callback));

  // When the connection to |releaser| is closed, ReleaseFileLock() will be
  // called. This will release the lock on the file and cause
  // |on_close_callback| to be run.
  media::mojom::CdmFileReleaserAssociatedPtrInfo releaser;
  mojo::MakeStrongAssociatedBinding(
      std::make_unique<CdmFileReleaserImpl>(file_lock_key),
      mojo::MakeRequest(&releaser));
  std::move(callback).Run(Status::SUCCESS, std::move(file),
                          std::move(releaser));
}

void CdmStorageImpl::Close() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  delete this;
}
