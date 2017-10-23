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
                         CdmStorageImpl* cdm_storage)
    : file_name_(file_name), cdm_storage_(cdm_storage) {
  DVLOG(3) << __func__ << " " << file_name_;
}

CdmFileImpl::~CdmFileImpl() {
  DVLOG(3) << __func__ << " " << file_name_;

  cdm_storage_->CloseFile(file_name_);
  if (temp_file_opened_)
    cdm_storage_->CloseFile(GetTempFileName(file_name_));
}

void CdmFileImpl::OpenTempFileForWrite(OpenTempFileForWriteCallback callback) {
  DVLOG(3) << __func__ << " " << file_name_;

  temp_file_opened_ = true;
  cdm_storage_->OpenFile(
      GetTempFileName(file_name_),
      base::File::FLAG_CREATE_ALWAYS | base::File::FLAG_WRITE,
      std::move(callback));
}

void CdmFileImpl::RenameTempFileAndReopen(
    RenameTempFileAndReopenCallback callback) {
  DVLOG(3) << __func__ << " " << file_name_;

  temp_file_opened_ = false;
  cdm_storage_->RenameAndReopenFile(GetTempFileName(file_name_), file_name_,
                                    std::move(callback));
}

}  // namespace content
