// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiling_host/profiling_process_host.h"

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/profiling/memlog_sender.h"
#include "chrome/common/profiling/profiling_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"

namespace profiling {

namespace {

ProfilingProcessHost* pph_singleton = nullptr;

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
  Launch();
}

ProfilingProcessHost::~ProfilingProcessHost() {
  pph_singleton = nullptr;
}

// static
ProfilingProcessHost* ProfilingProcessHost::EnsureStarted() {
  static ProfilingProcessHost host;
  return &host;
}

// static
ProfilingProcessHost* ProfilingProcessHost::Get() {
  return pph_singleton;
}

// static
void ProfilingProcessHost::AddSwitchesToChildCmdLine(
    base::CommandLine* child_cmd_line) {
  // Watch out: will be called on different threads.
  ProfilingProcessHost* pph = ProfilingProcessHost::Get();
  if (!pph)
    return;
  pph->EnsureControlChannelExists();
  child_cmd_line->AppendSwitchASCII(switches::kMemlogPipe, pph->pipe_id_);
}

void ProfilingProcessHost::Launch() {
  base::Process process = base::Process::Current();
  pipe_id_ = base::IntToString(static_cast<int>(process.Pid()));

  mojo::edk::PlatformChannelPair control_channel;
  mojo::edk::HandlePassingInformation handle_passing_info;

  base::CommandLine profiling_cmd = MakeProfilingCommandLine(pipe_id_);

  // Keep the server handle, pass the client handle to the child.
  pending_control_connection_ = control_channel.PassServerHandle();
  control_channel.PrepareToPassClientHandleToChildProcess(&profiling_cmd,
                                                          &handle_passing_info);

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
  StartMemlogSender(pipe_id_);
}

void ProfilingProcessHost::EnsureControlChannelExists() {
  // May get called on different threads, we need to be on the IO thread to
  // work.
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::IO)) {
    content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::IO)
        ->PostTask(
            FROM_HERE,
            base::BindOnce(&ProfilingProcessHost::EnsureControlChannelExists,
                           base::Unretained(this)));
    return;
  }

  if (pending_control_connection_.is_valid())
    ConnectControlChannel();
}

void ProfilingProcessHost::ConnectControlChannel() {
  mojo::edk::OutgoingBrokerClientInvitation invitation;
  mojo::ScopedMessagePipeHandle control_pipe =
      invitation.AttachMessagePipe(kControlPipeName);

  invitation.Send(
      process_.Handle(),
      mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                  std::move(pending_control_connection_)));
  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::IO)
      ->PostTask(
          FROM_HERE,
          base::BindOnce(&ProfilingProcessHost::BindControlChannelOnIO,
                         base::Unretained(this), std::move(control_pipe)));
}

void ProfilingProcessHost::BindControlChannelOnIO(
    mojo::ScopedMessagePipeHandle control_pipe) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  profiling_control_.Bind(
      mojom::ProfilingControlPtrInfo(std::move(control_pipe), 0));


  // ERASMEE
  profiling_control_->AddNewSender(42);
}

}  // namespace profiling
