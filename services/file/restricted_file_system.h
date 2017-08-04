// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_FILE_RESTRICTED_FILE_SYSTEM_H_
#define SERVICES_FILE_RESTRICTED_FILE_SYSTEM_H_

#include "base/files/file_path.h"
#include "components/filesystem/public/interfaces/file.mojom.h"
#include "services/file/public/interfaces/restricted_file_system.mojom.h"

namespace filesystem {
class LockTable;
}

namespace file {

// Implementation of mojom::RestrictedFileSystem
class RestrictedFileSystemImpl : public mojom::RestrictedFileSystem {
 public:
  RestrictedFileSystemImpl(const base::FilePath& base_user_dir,
                       const scoped_refptr<filesystem::LockTable>& lock_table);
  ~RestrictedFileSystemImpl() override;

  // Overridden from mojom::RestrictedFileSystem:
  void CreateTempFile(::filesystem::mojom::FileRequest file,
                      CreateTempFileCallback callback) override;

 private:
  scoped_refptr<filesystem::LockTable> lock_table_;
  base::FilePath path_;

  DISALLOW_COPY_AND_ASSIGN(RestrictedFileSystemImpl);
};

}  // namespace file

#endif  // SERVICES_FILE_RESTRICTED_FILE_SYSTEM_H_
