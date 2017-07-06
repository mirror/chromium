// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiling_host/profiling_process_host.h"

#include "base/command_line.h"
#include "base/file_descriptor_store.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/profiling/memlog_sender.h"
#include "chrome/profiling/profiling_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/file_descriptor_info.h"
#include "content/public/common/content_switches.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/public/cpp/system/platform_handle.h"

#if defined(OS_LINUX)
#include "base/posix/global_descriptors.h"
#include "base/process/process_metrics.h"
#include "base/third_party/valgrind/valgrind.h"
#include "content/public/common/content_descriptors.h"

enum {
  kProfilingDataPipe = kContentIPCDescriptorMax + 113,
};
#endif

namespace profiling {

namespace {

ProfilingProcessHost* pph_singleton = nullptr;

#if defined(OS_LINUX)
bool IsRunningOnValgrind() {
  return RUNNING_ON_VALGRIND;
}
#endif

base::CommandLine MakeProfilingCommandLine(const std::string& pipe_id) {
  // Program name.
  base::FilePath child_path;
#if defined(OS_LINUX)
  // Use /proc/self/exe rather than our known binary path so updates
  // can't swap out the binary from underneath us.
  // When running under Valgrind, forking /proc/self/exe ends up forking the
  // Valgrind executable, which then crashes. However, it's almost safe to
  // assume that the updates won't happen while testing with Valgrind tools.
  if (!IsRunningOnValgrind())
    child_path = base::FilePath(base::kProcSelfExe);
#endif
  if (child_path.empty())
    base::PathService::Get(base::FILE_EXE, &child_path);
  base::CommandLine result(child_path);

  result.AppendSwitchASCII(switches::kProcessType, switches::kProfiling);
  result.AppendSwitchASCII(switches::kMemlogPipe, pipe_id);

#if defined(OS_WIN)
  // Windows needs prefetch arguments.
  result.AppendArg(switches::kPrefetchArgumentOther);
#endif

  return result;
}

}  // namespace

ProfilingProcessHost::ProfilingProcessHost() {
  pph_singleton = this;
}

ProfilingProcessHost::~ProfilingProcessHost() {
  pph_singleton = nullptr;
}

// static
ProfilingProcessHost* ProfilingProcessHost::EnsureStarted() {
  static ProfilingProcessHost host;
  if (!host.process_.IsValid())
    host.Launch();
  return &host;
}

// static
ProfilingProcessHost* ProfilingProcessHost::Get() {
  return pph_singleton;
}

// static
void ProfilingProcessHost::AddSwitchesToChildCmdLine(
    base::CommandLine* child_cmd_line,
    int child_process_id) {
  ProfilingProcessHost* pph = ProfilingProcessHost::Get();
  if (!pph)
    return;
  pph->EnsureMemlogExistOnIO();
  child_cmd_line->AppendSwitchASCII(switches::kMemlogPipe,
                                    base::IntToString(kProfilingDataPipe));

  LOG(ERROR) << "Browser: adding arg to : " << child_process_id;
}

#if defined(OS_POSIX) && !defined(OS_MACOSX)
void ProfilingProcessHost::GetAdditionalMappedFilesForChildProcess(
    const base::CommandLine& command_line,
    int child_process_id,
    content::FileDescriptorInfo* mappings) {
  ProfilingProcessHost* pph = ProfilingProcessHost::Get();
  if (!pph)
    return;

  pph->EnsureMemlogExistOnIO();
  mojo::edk::PlatformChannelPair channel;

  auto x = channel.PassClientHandle();
  mappings->Transfer(kProfilingDataPipe, base::ScopedFD(x.release().handle));

  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::IO)
      ->PostTask(FROM_HERE,
                 base::BindOnce(&ProfilingProcessHost::AddNewSenderOnIO,
                                base::Unretained(pph),
                                channel.PassServerHandle(), child_process_id));
  LOG(ERROR) << "Browser: starting with child id: " << child_process_id;
}
#endif  // defined(OS_POSIX) && !defined(OS_MACOSX)

void ProfilingProcessHost::AddNewSenderOnIO(
    mojo::edk::ScopedPlatformHandle handle,
    int child_process_id) {
  memlog_->AddNewSender(mojo::WrapPlatformFile(handle.release().handle),
                        child_process_id);
}

void ProfilingProcessHost::ConnectControlChannel() {
  mojo::edk::OutgoingBrokerClientInvitation invitation;

  mojo::ScopedMessagePipeHandle control_pipe =
      invitation.AttachMessagePipe(kControlPipeName);

  invitation.Send(process_.Handle(), mojo::edk::ConnectionParams(
                                         mojo::edk::TransportProtocol::kLegacy,
                                         control_channel_->PassServerHandle()));
  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::IO)
      ->PostTask(FROM_HERE, base::BindOnce(&ProfilingProcessHost::BindOnIO,
                                           base::Unretained(this),
                                           std::move(control_pipe)));
  control_channel_.reset();
}

void ProfilingProcessHost::EnsureMemlogExistOnIO() {
  // TODO(ajwong): This is a hack. Use a member variable for a flag.
  static bool been_called = false;
  if (!been_called) {
    been_called = true;
    ConnectControlChannel();
  }
}

void ProfilingProcessHost::BindOnIO(
    mojo::ScopedMessagePipeHandle control_pipe) {
  memlog_.Bind(mojom::MemlogPtrInfo(std::move(control_pipe), 0));
}

void ProfilingProcessHost::Launch() {
  mojo::edk::PlatformChannelPair initial_data_channel;
  control_channel_.reset(new mojo::edk::PlatformChannelPair());

  mojo::edk::HandlePassingInformation handle_passing_info;
  std::string initial_pipe_id =
      initial_data_channel.PrepareToPassClientHandleToChildProcessAsString(
          &handle_passing_info);

  base::CommandLine profiling_cmd = MakeProfilingCommandLine(initial_pipe_id);
  control_channel_->PrepareToPassClientHandleToChildProcess(
      &profiling_cmd, &handle_passing_info);

  base::LaunchOptions options;
#if defined(OS_WIN)
  options.handles_to_inherit = &handle_passing_info;
#elif defined(OS_POSIX)
  options.fds_to_remap = &handle_passing_info;
  options.kill_on_parent_death = true;
#else
#error Unsupported OS.
#endif

  process_ = base::LaunchProcess(profiling_cmd, options);
  base::ScopedPlatformHandle data_pipe(
      initial_data_channel.PassServerHandle().release().handle);
  StartMemlogSender(std::move(data_pipe));
  initial_data_channel.ChildProcessLaunched();
  control_channel_->ChildProcessLaunched();
}

}  // namespace profiling
