// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_SANDBOX_SANDBOX_INIT_H_
#define SERVICES_SERVICE_MANAGER_SANDBOX_SANDBOX_INIT_H_

#include "base/callback.h"
#include "base/files/file_path.h"
#include "build/build_config.h"
#include "services/service_manager/sandbox/export.h"
#include "services/service_manager/sandbox/sandbox_type.h"

namespace service_manager {

#if defined(OS_WIN)

// TODO(tsepez): move win code over after decoupling from content/.

#elif defined(OS_MACOSX)

// Initialize the sandbox of the given |sandbox_type|, optionally specifying a
// directory to allow access to. Note specifying a directory needs to be
// supported by the sandbox profile associated with the given |sandbox_type|.
//
// Returns true if the sandbox was initialized succesfully, false if an error
// occurred.  If process_type isn't one that needs sandboxing, no action is
// taken and true is always returned.
SERVICE_MANAGER_SANDBOX_EXPORT bool InitializeSandbox(
    service_manager::SandboxType sandbox_type,
    const base::FilePath& allowed_path,
    base::OnceClosure base::OnceClosure post_warmup_hook);

#elif defined(OS_LINUX) || defined(OS_NACL_NONSFI)

// TODO(tsepez): move linux code over after decoupling from content/.

#endif  // defined(OS_LINUX) || defined(OS_NACL_NONSFI)

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_SANDBOX_SANDBOX_INIT_H_
