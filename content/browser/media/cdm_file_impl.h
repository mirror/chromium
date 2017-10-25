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

// This class implements the media::mojom::CdmFile interface. It is associated
// with the same mojo pipe as the CdmStorageImpl object that creates it, so
// this object will go away if CdmStorageImpl is destroyed.
class CdmFileImpl final : public media::mojom::CdmFile {
 public:
  using OpenFileCallback = base::OnceCallback<void(base::File file)>;

  // The file system is different for each CDM and each origin. So track files
  // in use based on the tuple file system ID, origin, and file name.
  struct FileLockKey {
    FileLockKey(const std::string& file_system_id,
                const url::Origin& origin,
                const std::string& file_name);
    ~FileLockKey() = default;

    // Allow use as a key in std::set.
    bool operator<(const FileLockKey& other) const;

    std::string file_system_id;
    url::Origin origin;
    std::string file_name;
  };

  // Keep track of which files are opened.
  enum class FileState { kNone, kOpenForRead, kOpenForReadAndWrite };

  CdmFileImpl(const std::string& file_name,
              const url::Origin& origin,
              const std::string& file_system_id,
              scoped_refptr<storage::FileSystemContext> file_system_context,
              const std::string& file_system_root_uri);
  ~CdmFileImpl() final;

  // Called to open the file for read initially.
  void Initialize(OpenFileCallback file_opened_callback);

  // media::mojom::CdmFile implementation.
  void OpenTempFileForWriting(OpenTempFileForWritingCallback callback) final;
  void RenameTempFileAndReopen(RenameTempFileAndReopenCallback callback) final;

 private:
  using CreateOrOpenCallback = storage::AsyncFileUtil::CreateOrOpenCallback;

  // Open the file |file_name| using the flags provided in |file_flags|.
  // |callback| is called with the result.
  void OpenFile(const std::string& file_name,
                uint32_t file_flags,
                CreateOrOpenCallback callback);

  void OnFileOpenedForRead(OpenFileCallback callback,
                           base::File file,
                           const base::Closure& on_close_callback);
  void OnTempFileOpenedForWrite(OpenTempFileForWritingCallback callback,
                                base::File file,
                                const base::Closure& on_close_callback);
  void OnTempFileRenamed(RenameTempFileAndReopenCallback callback,
                         base::File::Error move_result);

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
  scoped_refptr<storage::FileSystemContext> file_system_context_;
  const std::string file_system_root_uri_;

  // Keep track of which files are opened.
  FileState state_ = FileState::kNone;

  // Callbacks required to close the file when it's no longer needed.
  base::Closure on_close_callback_;
  base::Closure temporary_file_on_close_callback_;

  THREAD_CHECKER(thread_checker_);
  base::WeakPtrFactory<CdmFileImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CdmFileImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CDM_FILE_IMPL_H_
