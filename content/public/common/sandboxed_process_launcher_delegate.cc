// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/sandboxed_process_launcher_delegate.h"

#include "build/build_config.h"

namespace content {

#if defined(OS_WIN)
bool SandboxedProcessLauncherDelegate::DisableDefaultPolicy() {
  return false;
}

bool SandboxedProcessLauncherDelegate::PreSpawnTarget(
    sandbox::TargetPolicy* policy) {
  return true;
}

void SandboxedProcessLauncherDelegate::PostSpawnTarget(
    base::ProcessHandle process) {}

bool SandboxedProcessLauncherDelegate::ShouldLaunchElevated() {
  return false;
}
#endif  // defined(OS_WIN)

#if defined(OS_LINUX)
ZygoteHandle SandboxedProcessLauncherDelegate::GetZygote() {
  return nullptr;
}
#endif  // defined(OS_LINUX)

#if defined(OS_POSIX)
base::EnvironmentMap SandboxedProcessLauncherDelegate::GetEnvironment() {
  return base::EnvironmentMap();
}
#endif  // defined(OS_POSIX)

}  // namespace content
