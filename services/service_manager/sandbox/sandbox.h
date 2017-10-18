// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICE_MANAGER_SANDBOX_SANDBOX_H_
#define SERVICE_MANAGER_SANDBOX_SANDBOX_H_

#include "build/build_config.h"
#include "services/service_manager/sandbox/export.h"
#include "services/service_manager/sandbox/sandbox_type.h"

#if defined(OS_MACOSX)
#include "base/callback.h"
#include "base/files/file_path.h"
#endif  // defined(OS_MACOSX)

namespace service_manager {

// Interface to the service manager sandboxes across the various platforms.
class SERVICE_MANAGER_SANDBOX_EXPORT Sandbox {
 public:
#if defined(OS_LINUX)
// TODO(tsepez): move linux code here.
#endif  // defined(OS_LINUX)

#if defined(OS_MACOSX)
  static bool Initialize(service_manager::SandboxType sandbox_type,
                         const base::FilePath& allowed_dir,
                         base::OnceClosure hook);
#endif  // defined(OS_MACOSX)

#if defined(OS_WIN)
// TODO(tsepez): move win code here.
#endif  // defined(OS_WIN)
};

}  // namespace service_manager

#endif  // SERVICE_MANAGER_SANDBOX_SANDBOX_H_
