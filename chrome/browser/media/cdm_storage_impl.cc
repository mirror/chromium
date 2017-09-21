// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/cdm_storage_impl.h"

#include <map>
#include <utility>

#include "base/callback.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "base/task_scheduler/post_task.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "media/base/key_system_names.h"
#include "media/cdm/cdm_paths.h"
#include "ppapi/shared_impl/ppapi_constants.h"
#include "storage/browser/fileapi/async_file_util.h"
#include "storage/browser/fileapi/file_system_context.h"
#include "storage/browser/fileapi/file_system_operation_context.h"
#include "storage/browser/fileapi/isolated_context.h"
#include "storage/browser/quota/quota_manager.h"
#include "third_party/widevine/cdm/widevine_cdm_common.h"

// Currently this uses the PepperPlugInFileSystem as the existing CDMs run
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
// A lock must be acquired before opening a file. The lock is held until
// the connection with the client is broken.
// Notes:
// - When multiple CDM instances are running in different profiles (e.g.
//   normal/incognito window, multiple profiles), we may be overlocking.
// - This map is never deleted. As entries are removed when the file is
//   closed, this map should never get too large.
using FileLockMap = std::map<std::string, base::Closure>;

FileLockMap* GetFileMap() {
  static auto* file_map = new FileLockMap();
  return file_map;
}

// Use a base::Lock() to ensure that only one instance accesses |file_map|
// at a time.
base::Lock* GetFileLock() {
  static auto* lock = new base::Lock();
  return lock;
}

bool AcquireFileLock(const std::string& file_name) {
  DVLOG(3) << __func__ << " file: " << file_name;
  if (file_name.empty())
    return false;

  base::AutoLock auto_lock(*GetFileLock());
  // Make sure there isn't an existing entry for this file.
  if (GetFileMap()->find(file_name) != GetFileMap()->end())
    return false;

  // Add a new entry.
  return GetFileMap()->emplace(file_name, base::Closure()).second;
}

void UpdateFileLock(const std::string& file_name, base::Closure callback) {
  DVLOG(3) << __func__ << " file: " << file_name;
  DCHECK(!file_name.empty());
  DCHECK(!GetFileMap()->empty());

  base::AutoLock auto_lock(*GetFileLock());
  auto entry = GetFileMap()->find(file_name);
  DCHECK(entry != GetFileMap()->end());
  entry->second = std::move(callback);
}

void ReleaseFileLock(const std::string& file_name) {
  DVLOG(3) << __func__ << " file: " << file_name;
  DCHECK(!file_name.empty());

  base::AutoLock auto_lock(*GetFileLock());
  auto entry = GetFileMap()->find(file_name);
  if (entry == GetFileMap()->end())
    return;

  if (entry->second)
    entry->second.Run();
  GetFileMap()->erase(file_name);
}

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
  binding_.set_connection_error_handler(base::BindOnce(
      &CdmStorageImpl::OnConnectionClosed, weak_factory_.GetWeakPtr()));
}

CdmStorageImpl::~CdmStorageImpl() {
  if (!file_name_.empty())
    ReleaseFileLock(file_name_);
}

void CdmStorageImpl::Initialize(const std::string& key_system) {
  DVLOG(3) << __func__ << " key system: " << key_system;

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

void CdmStorageImpl::Open(const std::string& file_name,
                          media::mojom::CdmFileReleaserPtr releaser,
                          OpenCallback callback) {
  DVLOG(3) << __func__ << " file: " << file_name;
  if (plugin_id_.empty()) {
    DVLOG(1) << "CdmStorageImpl not initialized properly.";
    std::move(callback).Run(Status::FAILURE, base::File());
    return;
  }

  if (!filesystem_context_) {
    DVLOG(1) << "Unable to get FileSystemContext.";
    std::move(callback).Run(Status::FAILURE, base::File());
    return;
  }

  if (!IsValidFileName(file_name)) {
    DVLOG(1) << "Invalid file name " << file_name;
    std::move(callback).Run(Status::FAILURE, base::File());
    return;
  }

  if (!AcquireFileLock(file_name)) {
    DVLOG(1) << "File " << file_name << " is already in use.";
    std::move(callback).Run(Status::IN_USE, base::File());
    return;
  }

  std::string fsid =
      storage::IsolatedContext::GetInstance()->RegisterFileSystemForVirtualPath(
          storage::kFileSystemTypePluginPrivate, ppapi::kPluginPrivateRootName,
          base::FilePath());
  if (!storage::ValidateIsolatedFileSystemId(fsid)) {
    DVLOG(1) << "Invalid file system ID.";
    ReleaseFileLock(file_name);
    std::move(callback).Run(Status::FAILURE, base::File());
    return;
  }

  releaser_ = std::move(releaser);
  file_name_ = file_name;

  // Grant full access of isolated filesystem to renderer process.
  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  policy->GrantCreateReadWriteFileSystem(child_process_id_, fsid);

  filesystem_context_->OpenPluginPrivateFileSystem(
      GURL(origin_.Serialize()), storage::kFileSystemTypePluginPrivate, fsid,
      plugin_id_, storage::OPEN_FILE_SYSTEM_CREATE_IF_NONEXISTENT,
      base::Bind(&CdmStorageImpl::OnFileSystemOpened,
                 weak_factory_.GetWeakPtr(), fsid, base::Passed(&callback)));
}

void CdmStorageImpl::OnFileSystemOpened(const std::string& fsid,
                                        OpenCallback callback,
                                        base::File::Error error) {
  DVLOG(3) << __func__;

  if (error != base::File::FILE_OK) {
    DVLOG(1) << "Unable to access the file system.";
    ReleaseFileLock(file_name_);
    std::move(callback).Run(Status::FAILURE, base::File());
    return;
  }

  std::string root = storage::GetIsolatedFileSystemRootURIString(
                         GURL(origin_.Serialize()), fsid, "pluginprivate")
                         .append(file_name_);
  storage::FileSystemURL file_url = filesystem_context_->CrackURL(GURL(root));
  storage::AsyncFileUtil* file_util = filesystem_context_->GetAsyncFileUtil(
      storage::kFileSystemTypePluginPrivate);
  std::unique_ptr<storage::FileSystemOperationContext> operation_context =
      base::WrapUnique(
          new storage::FileSystemOperationContext(filesystem_context_.get()));
  operation_context->set_allowed_bytes_growth(storage::QuotaManager::kNoLimit);
  int flags = base::File::FLAG_OPEN_ALWAYS | base::File::FLAG_READ |
              base::File::FLAG_WRITE;
  DVLOG(1) << "Opening " << file_url.DebugString();
  file_util->CreateOrOpen(
      std::move(operation_context), file_url, flags,
      base::Bind(&CdmStorageImpl::OnFileOpened, weak_factory_.GetWeakPtr(),
                 base::Passed(&callback)));
}

void CdmStorageImpl::OnFileOpened(OpenCallback callback,
                                  base::File file,
                                  const base::Closure& on_close_callback) {
  DVLOG(3) << __func__;

  if (!file.IsValid()) {
    DLOG(WARNING) << "Unable to open file " << file_name_ << ", error: "
                  << base::File::ErrorToString(file.error_details());
    ReleaseFileLock(file_name_);
    std::move(callback).Run(Status::FAILURE, base::File());
    return;
  }

  // When the connection to |releaser_| is closed, release the lock on the file.
  UpdateFileLock(file_name_, std::move(on_close_callback));
  releaser_.set_connection_error_handler(
      base::BindOnce(&ReleaseFileLock, file_name_));

  std::move(callback).Run(Status::SUCCESS, std::move(file));
}

void CdmStorageImpl::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  if (render_frame_host->GetProcess()->GetID() == child_process_id_) {
    DVLOG(1) << __func__ << " for proper child_process_id_";
    OnConnectionClosed();
  }
}

void CdmStorageImpl::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (navigation_handle->GetRenderFrameHost()->GetProcess()->GetID() ==
      child_process_id_) {
    DVLOG(1) << __func__ << " for proper child_process_id_";
    OnConnectionClosed();
  }
}

void CdmStorageImpl::OnConnectionClosed() {
  delete this;
}
