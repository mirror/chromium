// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/file_utilities_host_impl.h"

#include "base/files/file_util.h"
#include "content/browser/child_process_security_policy_impl.h"
#include "mojo/public/cpp/bindings/strong_binding.h"

namespace content {

FileUtilitiesHostImpl::FileUtilitiesHostImpl(int process_id)
    : process_id_(process_id) {}

FileUtilitiesHostImpl::~FileUtilitiesHostImpl() = default;

void FileUtilitiesHostImpl::Create(
    int process_id,
    blink::mojom::FileUtilitiesHostRequest request) {
  mojo::MakeStrongBinding(base::MakeUnique<FileUtilitiesHostImpl>(process_id),
                          std::move(request));
}

void FileUtilitiesHostImpl::GetFileInfo(const base::FilePath& path,
                                        GetFileInfoCallback callback) {
  base::File::Info info;
  base::File::Error status = base::File::FILE_OK;

  if (!ChildProcessSecurityPolicyImpl::GetInstance()->CanReadFile(process_id_,
                                                                  path)) {
    std::move(callback).Run(info, status);
    return;
  }

  if (!base::GetFileInfo(path, &info)) {
    status = base::File::FILE_ERROR_FAILED;
  }
  std::move(callback).Run(info, status);
}

}  // namespace content
