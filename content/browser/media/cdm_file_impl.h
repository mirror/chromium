// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_CDM_FILE_IMPL_H_
#define CONTENT_BROWSER_MEDIA_CDM_FILE_IMPL_H_

#include <string>

#include "base/callback_forward.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"

namespace content {
class CdmStorageImpl;

// This class implements the media::mojom::CdmFile interface. It is associated
// with the same mojo pipe as the CdmStorageImpl object that creates it, so
// this object will go away if CdmStorageImpl is destroyed.
class CdmFileImpl final : public media::mojom::CdmFile {
 public:
  CdmFileImpl(const std::string& file_name, CdmStorageImpl* cdm_storage);
  ~CdmFileImpl() override;

  // media::mojom::CdmFile implementation.
  void OpenTempFileForWrite(OpenTempFileForWriteCallback callback) final;
  void RenameTempFileAndReopen(RenameTempFileAndReopenCallback callback) final;

 private:
  const std::string file_name_;
  CdmStorageImpl* cdm_storage_;

  // If creating a temp file for writing, keep track that it's been requested.
  bool temp_file_opened_ = false;

  DISALLOW_COPY_AND_ASSIGN(CdmFileImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_CDM_FILE_IMPL_H_
