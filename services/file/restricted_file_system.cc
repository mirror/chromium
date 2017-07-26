// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/file/restricted_file_system.h"

#include "base/files/file.h"
// #include "base/files/file_path.h"
#include "base/files/file_util.h"
// #include "base/files/scoped_temp_dir.h"
// #include "base/memory/ptr_util.h"
// #include "base/strings/utf_string_conversions.h"
#include "components/filesystem/file_impl.h"
#include "components/filesystem/lock_table.h"
#include "components/filesystem/shared_temp_dir.h"
#include "components/filesystem/public/interfaces/types.mojom.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace file {

RestrictedFileSystemImpl::RestrictedFileSystemImpl(
    const base::FilePath& base_user_dir,
    const scoped_refptr<filesystem::LockTable>& lock_table)
    : lock_table_(lock_table), path_(base_user_dir) {
  base::CreateDirectory(path_);
  LOG(ERROR) << "RestrictedFileSystemImpl created";
}

RestrictedFileSystemImpl::~RestrictedFileSystemImpl() {}

void RestrictedFileSystemImpl::CreateTempFile(
    ::filesystem::mojom::FileRequest request,
    CreateTempFileCallback callback) {
  base::FilePath file_path;
  if (!base::CreateTemporaryFile(&file_path)) {
    base::ResetAndReturn(&callback).Run(filesystem::mojom::FileError::FAILED);
  }
  LOG(ERROR) << "Created file " << file_path.AsUTF8Unsafe();
  mojo::MakeStrongBinding(
      base::MakeUnique<filesystem::FileImpl>(
          file_path,
          (base::File::FLAG_OPEN | base::File::FLAG_READ |
           base::File::FLAG_WRITE | base::File::FLAG_APPEND |
           base::File::FLAG_DELETE_ON_CLOSE),
          scoped_refptr<filesystem::SharedTempDir>(), lock_table_),
      std::move(request));
  LOG(ERROR) << "Sending response";
  base::ResetAndReturn(&callback).Run(filesystem::mojom::FileError::OK);
  LOG(ERROR) << "Response dispatched";
}

}  // namespace file
