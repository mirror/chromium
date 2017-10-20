// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CDM_FILE_IMPL_H_
#define CONTENT_BROWSER_MEDIA_CDM_FILE_IMPL_H_

#include <string>

#include "base/callback_forward.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"

namespace content {
class CdmStorageImpl;

// This class implements the media::mojom::CdmFile interface. It is associated
// with the same mojo pipe as the CdmStorageImpl object that creates it, so
// this object will go away if CdmStorageImpl is destroyed.
class CdmFileImpl final : public media::mojom::CdmFile {
 public:
  CdmFileImpl(const std::string& file_name,
              CdmStorageImpl* cdm_storage,
              const base::Closure& on_close_callback,
              base::OnceClosure file_done_callback);
  ~CdmFileImpl() override;

  // media::mojom::CdmFile implementation.
  void OpenFileForWrite(OpenFileForWriteCallback callback) final;
  void DoneWriting(DoneWritingCallback callback) final;

 private:
  // Called when OpenFileForWrite() is complete.
  void OnFileOpenedForWrite(OpenFileForWriteCallback callback,
                            base::File file,
                            const base::Closure& on_close_callback);

  // Called after the temp file is renamed to the original file name
  // (step 1 of DoneWriting()).
  void OnFileRenamed(DoneWritingCallback callback, base::File::Error error);

  // Called when the original file has been reopened for reading
  // (step 2 of DoneWriting()).
  void OnFileOpenedForRead(DoneWritingCallback callback,
                           base::File file,
                           const base::Closure& on_close_callback);

  const std::string file_name_;
  CdmStorageImpl* cdm_storage_;
  base::Closure on_close_callback_;
  base::OnceClosure file_done_callback_;

  // After creating a temp file for writing, keep track of the close_callback
  // so it can be closed before the rename.
  base::Closure temp_file_close_callback_;

  base::WeakPtrFactory<CdmFileImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CdmFileImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CDM_FILE_IMPL_H_
