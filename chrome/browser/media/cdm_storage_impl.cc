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
#include "mojo/public/cpp/bindings/strong_binding.h"
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
// A lock must be acquired before opening a file to ensure that the file is not
// currently in use. Once the file is opened the file map must be updated to
// include the callback used to close the file. The lock is held until the file
// is closed (determined by the connection with the client being broken).
// Notes:
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

// As this is in the browser process, the file system is different for
// each plugin type and each origin. So track |file_name| as if it existed
// inside subdirectories.
std::string GetEntryName(const std::string& plugin_id,
                         const url::Origin& origin,
                         const std::string& file_name) {
  return plugin_id + "/" + origin.Serialize() + "/" + file_name;
}

bool AcquireFileLock(const std::string& plugin_id,
                     const url::Origin& origin,
                     const std::string& file_name) {
  DVLOG(3) << __func__ << " file: " << file_name;
  if (file_name.empty())
    return false;

  const std::string entry_name = GetEntryName(plugin_id, origin, file_name);
  base::AutoLock auto_lock(*GetFileLock());
  // Make sure there isn't an existing entry for this file.
  if (GetFileMap()->find(entry_name) != GetFileMap()->end())
    return false;

  // Add a new entry.
  return GetFileMap()->emplace(entry_name, base::Closure()).second;
}

void UpdateFileLock(const std::string& plugin_id,
                    const url::Origin& origin,
                    const std::string& file_name,
                    base::Closure callback) {
  DVLOG(3) << __func__ << " file: " << file_name;
  DCHECK(!file_name.empty());
  DCHECK(!GetFileMap()->empty());

  const std::string entry_name = GetEntryName(plugin_id, origin, file_name);
  base::AutoLock auto_lock(*GetFileLock());
  auto entry = GetFileMap()->find(entry_name);
  DCHECK(entry != GetFileMap()->end());
  entry->second = std::move(callback);
}

void ReleaseFileLock(const std::string& plugin_id,
                     const url::Origin& origin,
                     const std::string& file_name) {
  DVLOG(3) << __func__ << " file: " << file_name;
  DCHECK(!file_name.empty());

  const std::string entry_name = GetEntryName(plugin_id, origin, file_name);
  base::AutoLock auto_lock(*GetFileLock());
  auto entry = GetFileMap()->find(entry_name);
  if (entry == GetFileMap()->end())
    return;

  if (entry->second)
    entry->second.Run();
  GetFileMap()->erase(entry_name);
}

// mojom::CdmStorage returns a mojom::CdmFileReleaser reference to keep track
// of the file being used. This object is created when the file is being
// passed to the client. When the client is done using the file, the connection
// should be broken and this will release the lock held on the file.
class CdmFileReleaserImpl final : public media::mojom::CdmFileReleaser {
 public:
  CdmFileReleaserImpl(const std::string& plugin_id,
                      const url::Origin& origin,
                      const std::string& file_name)
      : plugin_id_(plugin_id), origin_(origin), file_name_(file_name) {
    DVLOG(1) << __func__;
  }

  ~CdmFileReleaserImpl() override {
    DVLOG(1) << __func__;
    ReleaseFileLock(plugin_id_, origin_, file_name_);
  }

 private:
  std::string plugin_id_;
  url::Origin origin_;
  std::string file_name_;

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
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  binding_.set_connection_error_handler(base::BindOnce(
      &CdmStorageImpl::OnConnectionClosed, weak_factory_.GetWeakPtr()));
}

CdmStorageImpl::~CdmStorageImpl() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  // TODO: Close open connections.
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

  if (!AcquireFileLock(plugin_id_, origin_, file_name)) {
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
    ReleaseFileLock(plugin_id_, origin_, file_name);
    std::move(callback).Run(Status::FAILURE, base::File(), nullptr);
    return;
  }

  // Grant full access of isolated filesystem to renderer process.
  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  policy->GrantCreateReadWriteFileSystem(child_process_id_, fsid);

  filesystem_context_->OpenPluginPrivateFileSystem(
      GURL(origin_.Serialize()), storage::kFileSystemTypePluginPrivate, fsid,
      plugin_id_, storage::OPEN_FILE_SYSTEM_CREATE_IF_NONEXISTENT,
      base::Bind(&CdmStorageImpl::OnFileSystemOpened,
                 weak_factory_.GetWeakPtr(), file_name, fsid,
                 base::Passed(&callback)));
}

void CdmStorageImpl::OnFileSystemOpened(const std::string& file_name,
                                        const std::string& fsid,
                                        OpenCallback callback,
                                        base::File::Error error) {
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (error != base::File::FILE_OK) {
    DVLOG(1) << "Unable to access the file system.";
    ReleaseFileLock(plugin_id_, origin_, file_name);
    std::move(callback).Run(Status::FAILURE, base::File(), nullptr);
    return;
  }

  std::string root = storage::GetIsolatedFileSystemRootURIString(
                         GURL(origin_.Serialize()), fsid, "pluginprivate")
                         .append(file_name);
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
                 file_name, base::Passed(&callback)));
}

void CdmStorageImpl::OnFileOpened(const std::string& file_name,
                                  OpenCallback callback,
                                  base::File file,
                                  const base::Closure& on_close_callback) {
  // |on_close_callback| should be called after the |file| is closed in the
  // child process. See AsyncFileUtil for details.
  DVLOG(3) << __func__;
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (!file.IsValid()) {
    DLOG(WARNING) << "Unable to open file " << file_name << ", error: "
                  << base::File::ErrorToString(file.error_details());
    ReleaseFileLock(plugin_id_, origin_, file_name);
    std::move(callback).Run(Status::FAILURE, base::File(), nullptr);
    return;
  }

  UpdateFileLock(plugin_id_, origin_, file_name, std::move(on_close_callback));

  // When the connection to |releaser| is closed, release the lock on the file.
  // This will cause |on_close_callback| to be run.
  media::mojom::CdmFileReleaserPtr releaser;
  mojo::MakeStrongBinding(
      base::MakeUnique<CdmFileReleaserImpl>(plugin_id_, origin_, file_name),
      mojo::MakeRequest(&releaser));
  std::move(callback).Run(Status::SUCCESS, std::move(file),
                          std::move(releaser));
}

void CdmStorageImpl::RenderFrameDeleted(
    content::RenderFrameHost* render_frame_host) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (render_frame_host->GetProcess()->GetID() == child_process_id_) {
    DVLOG(1) << __func__ << " for proper child_process_id_";
    OnConnectionClosed();
  }
}

void CdmStorageImpl::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (navigation_handle->GetRenderFrameHost()->GetProcess()->GetID() ==
      child_process_id_) {
    DVLOG(1) << __func__ << " for proper child_process_id_";
    OnConnectionClosed();
  }
}

void CdmStorageImpl::OnConnectionClosed() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  delete this;
}
