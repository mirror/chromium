// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_CHROME_FILE_UTIL_ZIP_FILE_CREATOR_IMPL_H_
#define CHROME_SERVICES_CHROME_FILE_UTIL_ZIP_FILE_CREATOR_IMPL_H_

#include <vector>

#include "chrome/services/chrome_file_util/public/interfaces/zip_file_creator.mojom.h"
#include "components/filesystem/public/interfaces/directory.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace base {
class FilePath;
}

namespace chrome {

class ZipFileCreatorImpl : public chrome::mojom::ZipFileCreator {
 public:
  explicit ZipFileCreatorImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~ZipFileCreatorImpl() override;

 private:
  // chrome::mojom::ZipFileCreator:
  void CreateZipFile(filesystem::mojom::DirectoryPtr source_dir_mojo,
                     const base::FilePath& source_dir,
                     const std::vector<base::FilePath>& source_relative_paths,
                     base::File zip_file,
                     CreateZipFileCallback callback) override;

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(ZipFileCreatorImpl);
};

}  // namespace chrome

#endif  // CHROME_SERVICES_CHROME_FILE_UTIL_ZIP_FILE_CREATOR_IMPL_H_
