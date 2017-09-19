// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/sandbox_policy_fuchsia.h"

#include <launchpad/launchpad.h>
#include <zircon/processargs.h>

#include "base/command_line.h"
#include "content/public/common/content_switches.h"

namespace content {

uint32_t GetClonePolicyForSandbox(content::SandboxType type) {
  bool enable_sandbox =
      type != SANDBOX_TYPE_NO_SANDBOX &&
      !base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kNoSandbox);
  if (enable_sandbox) {
    return LP_CLONE_FDIO_STDIO;
  } else {
    return LP_CLONE_FDIO_NAMESPACE | LP_CLONE_DEFAULT_JOB | LP_CLONE_FDIO_CWD |
           LP_CLONE_FDIO_STDIO;
  }
}

}  // namespace content
