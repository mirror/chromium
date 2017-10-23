// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_SERVICES_CHROME_FILE_UTIL_FILE_PATCHER_IMPL_H_
#define CHROME_SERVICES_CHROME_FILE_UTIL_FILE_PATCHER_IMPL_H_

#include <memory>

#include "base/files/file.h"
#include "base/macros.h"
#include "chrome/services/chrome_file_util/public/interfaces/file_patcher.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace chrome {

class FilePatcherImpl : public chrome::mojom::FilePatcher {
 public:
  explicit FilePatcherImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~FilePatcherImpl() override;

 private:
  // chrome::mojom::FilePatcher:
  void PatchFileBsdiff(base::File input_file,
                       base::File patch_file,
                       base::File output_file,
                       PatchFileBsdiffCallback callback) override;
  void PatchFileCourgette(base::File input_file,
                          base::File patch_file,
                          base::File output_file,
                          PatchFileCourgetteCallback callback) override;

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;

  DISALLOW_COPY_AND_ASSIGN(FilePatcherImpl);
};

}  // namespace chrome

#endif  // CHROME_SERVICES_CHROME_FILE_UTIL_FILE_PATCHER_IMPL_H_
