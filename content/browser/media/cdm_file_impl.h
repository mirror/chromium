// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CDM_FILE_IMPL_H_
#define CONTENT_BROWSER_MEDIA_CDM_FILE_IMPL_H_

#include <string>

#include "base/callback_forward.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"
#include "storage/browser/fileapi/async_file_util.h"
#include "url/origin.h"

namespace storage {
class FileSystemContext;
class FileSystemURL;
}  // namespace storage

namespace content {

// This class implements the media::mojom::CdmFile interface. It is linked
// with the CdmStorageImpl object that creates it, so this object will go away
// if CdmStorageImpl is destroyed.
class CdmFileImpl final : public media::mojom::CdmFile {
 public:
  CdmFileImpl(const std::string& file_name,
              const url::Origin& origin,
              const std::string& file_system_id,
              const std::string& file_system_root_uri,
              scoped_refptr<storage::FileSystemContext> file_system_context);
  ~CdmFileImpl() final;

  // Called to open the file for read initially. Will create a file with
  // |file_name_| if it does not exist. Returns base::File::FILE_ERROR_IN_USE
  // if the file is in use by other CDMs or by the system. Note that |this|
  // can be destroyed while the operation is in progress, in which case
  // |file_opened_callback| will be called with base::File::FILE_ERROR_ABORT.
  using OpenFileCallback = base::OnceCallback<void(base::File file)>;
  void Initialize(OpenFileCallback file_opened_callback);

  // media::mojom::CdmFile implementation. As above, |this| can be destroyed
  // while the operations are in progress, and |callback| will be called with
  // base::File::FILE_ERROR_ABORT.
  void OpenTempFileForWriting(OpenTempFileForWritingCallback callback) final;
  void RenameTempFileAndReopen(RenameTempFileAndReopenCallback callback) final;

 private:
  using CreateOrOpenCallback = storage::AsyncFileUtil::CreateOrOpenCallback;

  // Keep track of which files are locked.
  enum class LockState { kNone, kOpenForRead, kOpenForReadAndWrite };

  // Open the file |file_name| using the flags provided in |file_flags|.
  // |callback| is called with the result.
  void OpenFile(const std::string& file_name,
                uint32_t file_flags,
                CreateOrOpenCallback callback);

  void OnFileOpenedForReading(base::File file,
                              const base::Closure& on_close_callback);
  void OnTempFileOpenedForWriting(base::File file,
                                  const base::Closure& on_close_callback);
  void OnTempFileRenamed(base::File::Error move_result);

  // Returns the FileSystemURL for the specified |file_name|.
  storage::FileSystemURL CreateFileSystemURL(const std::string& file_name);

  // Helper methods to lock and unlock a file.
  bool AcquireFileLock(const std::string& file_name);
  bool IsFileLockHeld(const std::string& file_name);
  void ReleaseFileLock(const std::string& file_name);

  // Names of the files this class represents.
  const std::string file_name_;
  const std::string temp_file_name_;

  // Files are stored in the PluginPrivateFileSystem. The following are needed
  // to access files.
  const url::Origin origin_;
  const std::string file_system_id_;
  const std::string file_system_root_uri_;
  scoped_refptr<storage::FileSystemContext> file_system_context_;

  // Keep track of which files are opened.
  LockState lock_state_ = LockState::kNone;

  // As only one open operation is allowed at a time, |pending_open_callback_|
  // keeps track of the callback to be called when the file is opened. This
  // ensures the callback is always called if we are destroyed while the open
  // operation is running.
  OpenFileCallback pending_open_callback_;

  // Callbacks required to close the file when it's no longer needed.
  // storage::AsyncFileUtil::CreateOrOpen() returns this callback on a
  // successful open along with the base::File object.
  base::Closure on_close_callback_;
  base::Closure temporary_file_on_close_callback_;

  THREAD_CHECKER(thread_checker_);
  base::WeakPtrFactory<CdmFileImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CdmFileImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CDM_FILE_IMPL_H_
