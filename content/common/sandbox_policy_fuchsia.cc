// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/sandbox_policy_fuchsia.h"

#include <launchpad/launchpad.h>
#include <zircon/processargs.h>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "content/public/common/content_switches.h"

namespace content {

void UpdateLaunchOptionsForSandbox(service_manager::SandboxType type,
                                   base::LaunchOptions* options) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kNoSandbox)) {
    type = service_manager::SANDBOX_TYPE_NO_SANDBOX;
  } else if (type != service_manager::SANDBOX_TYPE_NO_SANDBOX &&
             !base::IsPackageProcess()) {
    // TODO(crbug.com/750938): Remove this once package deployments become
    //                         mandatory.
    LOG(WARNING) << "Sandboxing was requested but is not available because";
    LOG(WARNING) << "the parent process is not hosted within a package.";
    type = service_manager::SANDBOX_TYPE_NO_SANDBOX;
  }

  if (type != service_manager::SANDBOX_TYPE_NO_SANDBOX) {
    // TODO(kmarshall): Build path mappings for each sandbox type.
    options->paths_to_transfer.insert(base::FilePath(base::kPackageRoot));
    base::FilePath temp_dir;
    base::GetTempDir(&temp_dir);
    options->paths_to_transfer.insert(temp_dir);
    options->clear_environ = true;
    options->clone_flags = LP_CLONE_FDIO_STDIO;
  } else {
    options->clone_flags =
        LP_CLONE_FDIO_NAMESPACE | LP_CLONE_DEFAULT_JOB | LP_CLONE_FDIO_STDIO;
    options->clear_environ = false;
  }
}

}  // namespace content
