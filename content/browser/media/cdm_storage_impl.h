// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CDM_STORAGE_IMPL_H_
#define CONTENT_BROWSER_MEDIA_CDM_STORAGE_IMPL_H_

#include <set>
#include <string>

#include "base/callback_forward.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/public/browser/frame_service_base.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"
#include "mojo/public/cpp/bindings/strong_associated_binding_set.h"

namespace storage {
class FileSystemContext;
}

namespace url {
class Origin;
}

namespace content {
class RenderFrameHost;

// This class implements the media::mojom::CdmStorage using the
// PluginPrivateFileSystem for backwards compatibility with CDMs running
// as a pepper plugin.
class CONTENT_EXPORT CdmStorageImpl final
    : public content::FrameServiceBase<media::mojom::CdmStorage> {
 public:
  // This must have same parameters as
  // storage::FileSystemContext::StatusCallback.
  using FileSystemOpenedCallback =
      base::OnceCallback<void(base::File::Error result)>;

  // This must match storage::AsyncFileUtil::CreateOrOpenCallback.
  // |on_close_callback| must be called after the |file| is closed.
  using FileOpenedCallback =
      base::Callback<void(base::File file,
                          const base::Closure& on_close_callback)>;

  // This must match storage::AsyncFileUtil::StatusCallback.
  using FileRenamedCallback = base::Callback<void(base::File::Error result)>;

  // The file system is different for each CDM and each origin. So track files
  // in use based on the tuple CDM file system ID, origin, and file name.
  struct FileLockKey {
    FileLockKey(const std::string& cdm_file_system_id,
                const url::Origin& origin,
                const std::string& file_name);
    ~FileLockKey() = default;

    // Allow use as a key in std::set.
    bool operator<(const FileLockKey& other) const;

    std::string cdm_file_system_id;
    url::Origin origin;
    std::string file_name;
  };

  // Check if |cdm_file_system_id| is valid.
  static bool IsValidCdmFileSystemId(const std::string& cdm_file_system_id);

  // Create a CdmStorageImpl object for |cdm_file_system_id| and bind it to
  // |request|.
  static void Create(RenderFrameHost* render_frame_host,
                     const std::string& cdm_file_system_id,
                     media::mojom::CdmStorageRequest request);

  // media::mojom::CdmStorage implementation.
  void Open(const std::string& file_name, OpenCallback callback) final;

  // Called to open |file_name| using |flags| in the previously opened
  // file system.
  void OpenFile(const std::string& file_name,
                int flags,
                FileOpenedCallback callback);

  // Called to rename |src_file_name| to |dest_file_name| in the previously
  // opened file system.
  void RenameFile(const std::string& src_file_name,
                  const std::string& dest_file_name,
                  FileRenamedCallback callback);

 private:
  // File system should only be opened once, so keep track if it has already
  // been opened (or is in the process of opening).
  enum class FileSystemState { kUnopened, kOpening, kOpened };

  CdmStorageImpl(RenderFrameHost* render_frame_host,
                 const std::string& cdm_file_system_id,
                 scoped_refptr<storage::FileSystemContext> file_system_context,
                 media::mojom::CdmStorageRequest request);
  ~CdmStorageImpl() final;

  // Helper functions to handle the asynchronous nature of Open().
  // Call sequence:
  //   Open() -> OpenFileSystem() -> OpenFileIfFileSystemOpened() ->
  //   OpenFile() -> OnFileOpened().
  void OpenFileIfFileSystemOpened(const FileLockKey& file_lock_key,
                                  OpenCallback callback,
                                  base::File::Error error);

  // Called when the requested file has been opened.
  void OnFileOpened(const FileLockKey& key,
                    OpenCallback callback,
                    base::File file,
                    const base::Closure& on_close_callback);

  // Called to open the file system.
  void OpenFileSystem(FileSystemOpenedCallback callback);
  void OnFileSystemOpened(base::File::Error error);

  // Files are stored in the PluginPrivateFileSystem, so keep track of the
  // CDM file system ID in order to open the files in the correct context.
  const std::string cdm_file_system_id_;
  scoped_refptr<storage::FileSystemContext> file_system_context_;

  // The PluginPrivateFileSystem only needs to be opened once.
  FileSystemState file_system_state_ = FileSystemState::kUnopened;
  base::File::Error file_system_opened_result_;
  std::vector<FileSystemOpenedCallback> pending_open_filesystem_callbacks_;

  // Once the PluginPrivateFileSystem is opened, keep track of the URI that
  // refers to it.
  std::string file_system_root_uri_;

  // This is the child process that will actually read and write the file(s)
  // returned, and it needs permission to access the file when it's opened.
  const int child_process_id_;

  // As a lock is taken on a file when Open() is called, it needs to be
  // released if the async operations to open the file haven't completed
  // by the time |this| is destroyed. So keep track of FileLockKey for files
  // that have a lock on them but failed to finish.
  std::set<FileLockKey> pending_open_;

  // Keep track of all media::mojom::CdmFile bindings, as each CdmFileImpl
  // object keeps a reference to |this|. If |this| goes away unexpectedly,
  // all remaining CdmFile bindings will be closed.
  mojo::StrongAssociatedBindingSet<media::mojom::CdmFile> cdm_file_bindings_;

  base::WeakPtrFactory<CdmStorageImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CdmStorageImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CDM_STORAGE_IMPL_H_
