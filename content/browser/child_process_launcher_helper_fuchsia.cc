// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/child_process_launcher_helper.h"

namespace content {
namespace internal {

void ChildProcessLauncherHelper::SetProcessPriorityOnLauncherThread(
    base::Process process,
    bool background,
    bool boost_for_pending_views) {
  NOTIMPLEMENTED();
}

base::TerminationStatus ChildProcessLauncherHelper::GetTerminationStatus(
    const ChildProcessLauncherHelper::Process& process,
    bool known_dead,
    int* exit_code) {
  NOTIMPLEMENTED();
  return base::GetTerminationStatus(0, 0);
}

// static
bool ChildProcessLauncherHelper::TerminateProcess(
    const base::Process& process, int exit_code, bool wait) {
  NOTIMPLEMENTED();
  return process.Terminate(exit_code, wait);
}

// static
void ChildProcessLauncherHelper::SetRegisteredFilesForService(
    const std::string& service_name,
    catalog::RequiredFileMap required_files) {
  NOTIMPLEMENTED();
}

void ChildProcessLauncherHelper::BeforeLaunchOnClientThread() {
  NOTIMPLEMENTED();
}

mojo::edk::ScopedPlatformHandle
ChildProcessLauncherHelper::PrepareMojoPipeHandlesOnClientThread() {
  NOTIMPLEMENTED();
  return mojo::edk::ScopedPlatformHandle();
}

std::unique_ptr<FileMappedForLaunch>
ChildProcessLauncherHelper::GetFilesToMap() {
  NOTIMPLEMENTED();
  return nullptr;
}

void ChildProcessLauncherHelper::BeforeLaunchOnLauncherThread(
    const PosixFileDescriptorInfo& files_to_register,
    base::LaunchOptions* options) {
  NOTIMPLEMENTED();
}

ChildProcessLauncherHelper::Process
ChildProcessLauncherHelper::LaunchProcessOnLauncherThread(
    const base::LaunchOptions& options,
    std::unique_ptr<FileMappedForLaunch> files_to_register,
    bool* is_synchronous_launch,
    int* launch_result) {
  NOTIMPLEMENTED();
  return Process();
}

void ChildProcessLauncherHelper::AfterLaunchOnLauncherThread(
    const ChildProcessLauncherHelper::Process& process,
    const base::LaunchOptions& options) {
  NOTIMPLEMENTED();
}

// static
void ChildProcessLauncherHelper::ForceNormalProcessTerminationSync(
    ChildProcessLauncherHelper::Process process) {
  NOTIMPLEMENTED();
}

}  // namespace internal
}  // namespace content
