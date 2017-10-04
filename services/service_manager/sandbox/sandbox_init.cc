// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/sandbox/sandbox_init.h"

#include <utility>

#include "base/build_config.h"
#include "services/service_manager/sandbox/sandbox_type.h"

#if defined(OS_MACOSX)
#include "services/service_manager/sandbox/mac/sandbox_mac.h"
#endif

namespace service_manager {

#if defined(OS_WIN)

// TODO(tsepez): small interface to win-specific code.

#elif defined(OS_MACOSX)

bool InitializeSandbox(service_manager::SandboxType sandbox_type,
                       const base::FilePath& allowed_dir,
                       base::OnceClosure hook) {
  // Warm up APIs before turning on the sandbox.
  Sandbox::SandboxWarmup(sandbox_type);

  // Execute the post warmup callback.
  if (!hook.is_null())
    std::move(hook).Run();

  // Actually sandbox the process.
  return Sandbox::EnableSandbox(sandbox_type, allowed_dir);
}

#elif defined(OS_LINUX) || defined(OS_NACL_NONSFI)

// TODO(tsepez): small interface to linux-specific code.

#endif  // defined(OS_LINUX) || defined(OS_NACL_NONSFI)

}  // namespace service_manager
