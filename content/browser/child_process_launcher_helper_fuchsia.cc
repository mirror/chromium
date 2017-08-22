// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/child_process_launcher_helper.h"

#include "base/command_line.h"
#include "base/debug/stack_trace.h"
#include "base/fuchsia/child_job.h"
#include "base/process/launch.h"
#include "content/browser/child_process_launcher_helper_posix.h"
#include "mojo/edk/embedder/platform_channel_pair.h"

#include <magenta/process.h>
#include <magenta/processargs.h>
#include <magenta/syscalls.h>

namespace content {
namespace internal {

void ChildProcessLauncherHelper::SetProcessPriorityOnLauncherThread(
    base::Process process,
    bool background,
    bool boost_for_pending_views,
    ChildProcessImportance importance) {
  DCHECK_CURRENTLY_ON(BrowserThread::PROCESS_LAUNCHER);
  // TODO(fuchsia): Implement this. (crbug.com/707031)
  NOTIMPLEMENTED();
}

base::TerminationStatus ChildProcessLauncherHelper::GetTerminationStatus(
    const ChildProcessLauncherHelper::Process& process,
    bool known_dead,
    int* exit_code) {
  return base::GetTerminationStatus(process.process.Handle(), exit_code);
}

// static
bool ChildProcessLauncherHelper::TerminateProcess(const base::Process& process,
                                                  int exit_code,
                                                  bool wait) {
  return process.Terminate(exit_code, wait);
}

// static
void ChildProcessLauncherHelper::SetRegisteredFilesForService(
    const std::string& service_name,
    catalog::RequiredFileMap required_files) {
  // TODO(fuchsia): Implement this. (crbug.com/707031)
  NOTIMPLEMENTED();
}

// static
void ChildProcessLauncherHelper::ResetRegisteredFilesForTesting() {
  // TODO(fuchsia): Implement this. (crbug.com/707031)
  NOTIMPLEMENTED();
}

void ChildProcessLauncherHelper::BeforeLaunchOnClientThread() {
  DCHECK_CURRENTLY_ON(client_thread_id_);
}

mojo::edk::ScopedPlatformHandle
ChildProcessLauncherHelper::PrepareMojoPipeHandlesOnClientThread() {
  DCHECK_CURRENTLY_ON(client_thread_id_);

  // By doing nothing here, StartLaunchOnClientThread() will construct a channel
  // pair instead.
  return mojo::edk::ScopedPlatformHandle();
}

std::unique_ptr<FileMappedForLaunch>
ChildProcessLauncherHelper::GetFilesToMap() {
  return std::unique_ptr<FileMappedForLaunch>();
}

void ChildProcessLauncherHelper::BeforeLaunchOnLauncherThread(
    const PosixFileDescriptorInfo& files_to_register,
    base::LaunchOptions* options) {
  DCHECK_CURRENTLY_ON(BrowserThread::PROCESS_LAUNCHER);

  mojo::edk::PlatformChannelPair::PrepareToPassHandleToChildProcess(
      mojo_client_handle(), command_line(), &options->handles_to_transfer);

  options->job_handle = base::GetChildProcessJob();
}

ChildProcessLauncherHelper::Process
ChildProcessLauncherHelper::LaunchProcessOnLauncherThread(
    const base::LaunchOptions& options,
    std::unique_ptr<FileMappedForLaunch> files_to_register,
    bool* is_synchronous_launch,
    int* launch_result) {
  DCHECK_CURRENTLY_ON(BrowserThread::PROCESS_LAUNCHER);
  DCHECK(mojo_client_handle().is_valid());

  // KM WIP sandbox TODO: don't set LP_CLONE_MXIO_NAMESPACE or LP_CLONE_MXIO_CWD
  // and maybe not LP_CLONE_DEFAULT_JOB either.

  base::CommandLine scrubbed_commandline({"/system/headless_shell"});
  for (size_t i = 1; i < command_line()->argv().size(); ++i) {
    auto next_arg = command_line()->argv()[i];
    if (next_arg.find("content-image-texture-target") == std::string::npos) {
      scrubbed_commandline.AppendSwitch(next_arg);
    }
  }

  Process process;
  process.process = base::LaunchProcess(scrubbed_commandline, options);
  if (process.process.IsValid()) {
    (mojo::edk::PlatformHandle) mojo_client_handle_.release();
  }

  return process;
}

void ChildProcessLauncherHelper::AfterLaunchOnLauncherThread(
    const ChildProcessLauncherHelper::Process& process,
    const base::LaunchOptions& options) {
  DCHECK_CURRENTLY_ON(BrowserThread::PROCESS_LAUNCHER);
}

// static
void ChildProcessLauncherHelper::ForceNormalProcessTerminationSync(
    ChildProcessLauncherHelper::Process process) {
  DCHECK_CURRENTLY_ON(BrowserThread::PROCESS_LAUNCHER);
  process.process.Terminate(RESULT_CODE_NORMAL_EXIT, false);
}

}  // namespace internal
}  // namespace content
