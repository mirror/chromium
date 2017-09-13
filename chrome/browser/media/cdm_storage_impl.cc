// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/cdm_storage_impl.h"

#include <map>
#include <utility>

#include "base/callback.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/synchronization/lock.h"
#include "base/task_scheduler/post_task.h"
#include "content/public/browser/child_process_security_policy.h"
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
// Close() is called. base::Closure is a callback to be called when the
// lock is released. It defaults to NOP unless set using UpdateFileLock().
// Notes:
// - When multiple CDM instances are running in different profiles (e.g.
//   normal/incognito window, multiple profiles), we may be overlocking.
// - This map is never deleted. As entries are removed when the file is
//   closed, this map should never get too large.
using FileLockMap = std::map<std::string, base::Closure>;
static base::LazyInstance<FileLockMap>::Leaky file_map;

// Use a base::Lock() to ensure that only one instance accesses |file_map|
// at a time.
static base::LazyInstance<base::Lock>::Leaky file_map_lock_;

bool AcquireFileLock(const std::string& file_name) {
  DVLOG(3) << __func__ << " file: " << file_name;
  if (file_name.empty())
    return false;

  base::AutoLock auto_lock(file_map_lock_.Get());
  // Make sure there isn't an existing entry for this file.
  if (file_map.Get().find(file_name) != file_map.Get().end())
    return false;

  // Add a new entry.
  return file_map.Get().emplace(file_name, base::Closure()).second;
}

void UpdateFileLock(const std::string& file_name, base::Closure callback) {
  DVLOG(3) << __func__ << " file: " << file_name;
  DCHECK(!file_name.empty());
  DCHECK(!file_map.Get().empty());

  base::AutoLock auto_lock(file_map_lock_.Get());
  auto entry = file_map.Get().find(file_name);
  DCHECK(entry != file_map.Get().end());
  entry->second = std::move(callback);
}

bool ReleaseFileLock(const std::string& file_name) {
  DVLOG(3) << __func__ << " file: " << file_name;
  DCHECK(!file_name.empty());

  base::AutoLock auto_lock(file_map_lock_.Get());
  auto entry = file_map.Get().find(file_name);
  if (entry == file_map.Get().end())
    return false;

  if (entry->second)
    entry->second.Run();
  file_map.Get().erase(file_name);
  return true;
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
  new CdmStorageImpl(render_frame_host->GetProcess(),
                     render_frame_host->GetLastCommittedOrigin(),
                     std::move(filesystem_context), std::move(request));
}

CdmStorageImpl::CdmStorageImpl(
    content::RenderProcessHost* render_process_host,
    const url::Origin& origin,
    scoped_refptr<storage::FileSystemContext> filesystem_context,
    media::mojom::CdmStorageRequest request)
    : render_process_host_(render_process_host),
      origin_(origin),
      binding_(this, std::move(request)),
      filesystem_context_(std::move(filesystem_context)),
      weak_factory_(this) {
  binding_.set_connection_error_handler(base::BindOnce(
      &CdmStorageImpl::OnConnectionClosed, weak_factory_.GetWeakPtr()));
}

CdmStorageImpl::~CdmStorageImpl() {}

void CdmStorageImpl::Initialize(const std::string& key_system,
                                InitializeCallback callback) {
  DVLOG(3) << __func__ << " key system: " << key_system;

  if (key_system == kWidevineKeySystem) {
    plugin_id_ = GenerateFileSystemId(kWidevineCdmPluginMimeType);
    std::move(callback).Run(true);
    return;
  }

  if (media::IsExternalClearKey(key_system)) {
    plugin_id_ = GenerateFileSystemId(media::kClearKeyCdmPepperMimeType);
    std::move(callback).Run(true);
    return;
  }

  DLOG(WARNING) << "Unrecognized key system " << key_system;
  std::move(callback).Run(false);
}

void CdmStorageImpl::Open(const std::string& name, OpenCallback callback) {
  DVLOG(3) << __func__ << " file: " << name;
  DCHECK(!plugin_id_.empty()) << "CdmStorageImpl not initialized properly.";

  if (!filesystem_context_) {
    DVLOG(1) << "Unable to get FileSystemContext.";
    std::move(callback).Run(Status::FAILURE, base::File());
    return;
  }

  if (!IsValidFileName(name)) {
    DVLOG(1) << "Invalid file name " << name;
    std::move(callback).Run(Status::FAILURE, base::File());
    return;
  }

  if (!AcquireFileLock(name)) {
    DVLOG(1) << "File " << name << " is already in use.";
    std::move(callback).Run(Status::IN_USE, base::File());
    return;
  }

  std::string fsid =
      storage::IsolatedContext::GetInstance()->RegisterFileSystemForVirtualPath(
          storage::kFileSystemTypePluginPrivate, ppapi::kPluginPrivateRootName,
          base::FilePath());
  if (!storage::ValidateIsolatedFileSystemId(fsid)) {
    DVLOG(1) << "Invalid file system ID.";
    ReleaseFileLock(name);
    std::move(callback).Run(Status::FAILURE, base::File());
    return;
  }

  // Grant full access of isolated filesystem to renderer process.
  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  policy->GrantCreateReadWriteFileSystem(render_process_host_->GetID(), fsid);

  filesystem_context_->OpenPluginPrivateFileSystem(
      GURL(origin_.Serialize()), storage::kFileSystemTypePluginPrivate, fsid,
      plugin_id_, storage::OPEN_FILE_SYSTEM_CREATE_IF_NONEXISTENT,
      base::Bind(&CdmStorageImpl::OnFileSystemOpened,
                 weak_factory_.GetWeakPtr(), name, fsid,
                 base::Passed(&callback)));
}

void CdmStorageImpl::Close(const std::string& name, CloseCallback callback) {
  DVLOG(3) << __func__ << " file: " << name;

  // Caller has already closed the file, so simply run the on_close_callback
  // provided when the file was opened and release the file lock so that the
  // file can be opened again.
  std::move(callback).Run(ReleaseFileLock(name));
}

void CdmStorageImpl::OnFileSystemOpened(const std::string& name,
                                        const std::string& fsid,
                                        OpenCallback callback,
                                        base::File::Error error) {
  DVLOG(3) << __func__;

  if (error != base::File::FILE_OK) {
    DVLOG(1) << "Unable to access the file system.";
    ReleaseFileLock(name);
    std::move(callback).Run(Status::FAILURE, base::File());
    return;
  }

  std::string root = storage::GetIsolatedFileSystemRootURIString(
                         GURL(origin_.Serialize()), fsid, "pluginprivate")
                         .append(name);
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
                 name, base::Passed(&callback)));
}

void CdmStorageImpl::OnFileOpened(const std::string& name,
                                  OpenCallback callback,
                                  base::File file,
                                  const base::Closure& on_close_callback) {
  DVLOG(3) << __func__;

  if (!file.IsValid()) {
    DLOG(WARNING) << "Unable to open file " << name << ", error: "
                  << base::File::ErrorToString(file.error_details());
    ReleaseFileLock(name);
    std::move(callback).Run(Status::FAILURE, base::File());
    return;
  }

  UpdateFileLock(name, std::move(on_close_callback));
  std::move(callback).Run(Status::SUCCESS, std::move(file));
}

void CdmStorageImpl::OnConnectionClosed() {
  delete this;
}
