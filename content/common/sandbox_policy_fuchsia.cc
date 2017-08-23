// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/sandbox_policy_fuchsia.h"

#include <launchpad/launchpad.h>
#include <magenta/processargs.h>

namespace content {

uint32_t GetClonePolicyForSandbox(content::SandboxType type) {
  switch (type) {
    case SANDBOX_TYPE_NO_SANDBOX:
      return LP_CLONE_MXIO_NAMESPACE | LP_CLONE_DEFAULT_JOB | LP_CLONE_MXIO_CWD;
    default:
      return 0;  // No privileges.
  }
}

}  // namespace content
