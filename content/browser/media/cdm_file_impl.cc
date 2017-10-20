// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/media/cdm_file_impl.h"

#include <utility>

#include "base/callback.h"
#include "base/logging.h"
#include "content/browser/media/cdm_storage_impl.h"

namespace content {

namespace {

const char kTemporaryFilePrefix[] = "_";

std::string GetTempFileName(const std::string& file_name) {
  return kTemporaryFilePrefix + file_name;
}

}  // namespace

CdmFileImpl::CdmFileImpl(const std::string& file_name,
                         CdmStorageImpl* cdm_storage,
                         const base::Closure& on_close_callback,
                         base::OnceClosure file_done_callback)
    : file_name_(file_name),
      cdm_storage_(cdm_storage),
      on_close_callback_(on_close_callback),
      file_done_callback_(std::move(file_done_callback)),
      weak_factory_(this) {
  DVLOG(3) << __func__ << " " << file_name_;
}

CdmFileImpl::~CdmFileImpl() {
  DVLOG(3) << __func__ << " " << file_name_;

  if (on_close_callback_)
    std::move(on_close_callback_).Run();
  if (file_done_callback_)
    std::move(file_done_callback_).Run();
}

void CdmFileImpl::OpenFileForWrite(OpenFileForWriteCallback callback) {
  DVLOG(3) << __func__ << " " << file_name_;

  cdm_storage_->OpenFile(
      GetTempFileName(file_name_),
      base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE,
      base::Bind(&CdmFileImpl::OnFileOpenedForWrite, weak_factory_.GetWeakPtr(),
                 base::Passed(&callback)));
}

void CdmFileImpl::OnFileOpenedForWrite(OpenFileForWriteCallback callback,
                                       base::File file,
                                       const base::Closure& on_close_callback) {
  DVLOG(3) << __func__ << " " << file_name_;

  temp_file_close_callback_ = std::move(on_close_callback);
  std::move(callback).Run(std::move(file));
}

void CdmFileImpl::DoneWriting(DoneWritingCallback callback) {
  DVLOG(3) << __func__ << " " << file_name_;

  // Run the close_callback for the original file and for the temporary file
  // just written to. The caller should have close both base::File objects
  // prior to calling this method.
  if (on_close_callback_)
    std::move(on_close_callback_).Run();
  if (temp_file_close_callback_)
    std::move(temp_file_close_callback_).Run();

  cdm_storage_->RenameFile(
      GetTempFileName(file_name_), file_name_,
      base::Bind(&CdmFileImpl::OnFileRenamed, weak_factory_.GetWeakPtr(),
                 base::Passed(&callback)));
}

void CdmFileImpl::OnFileRenamed(DoneWritingCallback callback,
                                base::File::Error error) {
  DVLOG(3) << __func__ << " " << file_name_;

  if (error != base::File::FILE_OK) {
    std::move(callback).Run(base::File(error));
    return;
  }

  cdm_storage_->OpenFile(
      file_name_, base::File::FLAG_OPEN_ALWAYS | base::File::FLAG_READ,
      base::Bind(&CdmFileImpl::OnFileOpenedForRead, weak_factory_.GetWeakPtr(),
                 base::Passed(&callback)));
}

void CdmFileImpl::OnFileOpenedForRead(DoneWritingCallback callback,
                                      base::File file,
                                      const base::Closure& on_close_callback) {
  DVLOG(3) << __func__ << " " << file_name_;

  on_close_callback_ = std::move(on_close_callback);
  std::move(callback).Run(std::move(file));
}

}  // namespace content
