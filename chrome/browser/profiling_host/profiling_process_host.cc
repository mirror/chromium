// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiling_host/profiling_process_host.h"

#include "base/command_line.h"
#include "base/path_service.h"
#include "base/process/launch.h"
#include "base/strings/string_number_conversions.h"
#include "build/build_config.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/profiling/memlog_sender.h"
#include "chrome/common/profiling/profiling_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/public/cpp/system/platform_handle.h"

#if defined(OS_POSIX)
#include "base/files/scoped_file.h"
#include "base/process/process_metrics.h"
#include "base/third_party/valgrind/valgrind.h"
#include "chrome/common/profiling/profiling_constants.h"
#include "chrome/profiling/profiling_main.h"
#include "content/public/browser/file_descriptor_info.h"

#include "content/public/browser/browser_child_process_host.h"
#include "content/public/browser/browser_child_process_host_delegate.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/common/sandboxed_process_launcher_delegate.h"
#include "ipc/ipc_message.h"
#include "ui/base/ui_base_switches.h"
#endif

namespace profiling {

namespace {

ProfilingProcessHost* pph_singleton = nullptr;

std::atomic_int num_pipes_created{0};

#if defined(OS_POSIX)
#if defined(OS_LINUX)
bool IsRunningOnValgrind() {
  return RUNNING_ON_VALGRIND;
}
#endif

class ProfilingHostDelegate : public content::BrowserChildProcessHostDelegate {
 public:
  ~ProfilingHostDelegate() override {}
  bool OnMessageReceived(const IPC::Message& message) override { return true; }
};
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

  result.AppendSwitchASCII(switches::kProcessType, switches::kUtilityProcess);
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
  EnsureStarted();

  // TODO(ajwong): Figure out how to trace the zygote process.
  if (child_cmd_line->GetSwitchValueASCII(switches::kProcessType) ==
      switches::kZygoteProcess) {
    return;
  }

  // Watch out: will be called on different threads.
  ProfilingProcessHost* pph = ProfilingProcessHost::Get();
  if (!pph)
    return;
  pph->EnsureControlChannelExists();
  LOG(ERROR) << "Num data pipes created: "
             << num_pipes_created.fetch_add(1, std::memory_order_relaxed);

  // TODO(brettw) this isn't correct for Posix. Redo when we can shave over
  // Mojo
  child_cmd_line->AppendSwitchASCII(switches::kMemlogPipe, pph->pipe_id_);
}

#if defined(OS_POSIX) && !defined(OS_MACOSX)
void ProfilingProcessHost::GetAdditionalMappedFilesForChildProcess(
    const base::CommandLine& command_line,
    int child_process_id,
    content::FileDescriptorInfo* mappings) {
  EnsureStarted();

  // TODO(ajwong): Figure out how to trace the zygote process.
  if (command_line.GetSwitchValueASCII(switches::kProcessType) ==
      switches::kZygoteProcess) {
    return;
  }

  ProfilingProcessHost* pph = ProfilingProcessHost::Get();
  if (!pph)
    return;

  pph->EnsureControlChannelExists();

  mojo::edk::PlatformChannelPair data_channel;
  mappings->Transfer(
      kProfilingDataPipe,
      base::ScopedFD(data_channel.PassClientHandle().release().handle));

  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::IO)
      ->PostTask(
          FROM_HERE,
          base::BindOnce(&ProfilingProcessHost::AddNewSenderOnIO,
                         base::Unretained(pph), data_channel.PassServerHandle(),
                         child_process_id));
}
#endif  // defined(OS_POSIX) && !defined(OS_MACOSX)

void ProfilingProcessHost::Launch() {
  mojo::edk::PlatformChannelPair control_channel;
  mojo::edk::HandlePassingInformation handle_passing_info;

  // Create the socketpair for the low level memlog pipe.
  mojo::edk::PlatformChannelPair data_channel;
  pipe_id_ = data_channel.PrepareToPassClientHandleToChildProcessAsString(
      &handle_passing_info);

  mojo::edk::ScopedPlatformHandle child_end = data_channel.PassClientHandle();

  base::CommandLine profiling_cmd = MakeProfilingCommandLine(pipe_id_);

  // Keep the server handle, pass the client handle to the child.
  pending_control_connection_ = control_channel.PassServerHandle();
  /*
    control_channel.PrepareToPassClientHandleToChildProcess(&profiling_cmd,
                                                            &handle_passing_info);
  */

  // TODO(ajwong): Why did I do this?
  mojo::edk::ScopedPlatformHandle control_child_end =
      control_channel.PassClientHandle();
  handle_passing_info.push_back(
      std::pair<int, int>(control_child_end.get().handle, 20003));

#if !defined(OS_ANDROID)
  base::LaunchOptions options;
#if defined(OS_WIN)
  options.handles_to_inherit = &handle_passing_info;
#elif defined(OS_POSIX)
  options.fds_to_remap = &handle_passing_info;
  options.kill_on_parent_death = true;
#else
#error Unsupported OS.
#endif

//  process_ = base::LaunchProcess(profiling_cmd, options);
//  StartMemlogSender(pipe_id_);
#endif

  LOG(ERROR) << "PPH Launch **--**--**--**--**--**~~~ "
             << profiling_cmd.GetCommandLineString();
  StartMemlogSender(pipe_id_);
  // process_ = base::LaunchProcess(profiling_cmd, options);
  //  CHECK(process_.IsValid());
  static ProfilingHostDelegate delegate;
  content::BrowserChildProcessHost* host =
      content::BrowserChildProcessHost::Create(
          // TODO(ajwong): Clearly bad. Base off of PROCESS_TYPE_CONTENT_END and
          // don't clash with NACL.
          static_cast<content::ProcessType>(2000), &delegate);
  process_ = base::Process(host->GetData().handle);
  LOG(ERROR) << "PPH BCH requested **--**--**--**--**--**~~~ ";
  content::BrowserThread::GetTaskRunnerForThread(content::BrowserThread::IO)
      ->PostTask(
          FROM_HERE,
          base::BindOnce(
              &content::BrowserChildProcessHost::Launch, base::Unretained(host),
              base::MakeUnique<content::SandboxedProcessLauncherDelegate>(),
              base::MakeUnique<base::CommandLine>(profiling_cmd), true));
  LOG(ERROR) << "PPH Launched On IO **--**--**--**--**--**~~~ "
             << profiling_cmd.GetCommandLineString();
  StartMemlogSender(
      base::IntToString(data_channel.PassServerHandle().release().handle));
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
    ConnectControlChannelOnIO();
}

// This must be called before the client attempts to connect to the control
// pipe.
void ProfilingProcessHost::ConnectControlChannelOnIO() {
  mojo::edk::OutgoingBrokerClientInvitation invitation;
  mojo::ScopedMessagePipeHandle control_pipe =
      invitation.AttachMessagePipe(kProfilingControlPipeName);

  invitation.Send(
      process_.Handle(),
      mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                  std::move(pending_control_connection_)));
  profiling_control_.Bind(
      mojom::ProfilingControlPtrInfo(std::move(control_pipe), 0));

  StartProfilingMojo();
}

void ProfilingProcessHost::AddNewSenderOnIO(
    mojo::edk::ScopedPlatformHandle handle,
    int child_process_id) {
  profiling_control_->AddNewSender(
      mojo::WrapPlatformFile(handle.release().handle), child_process_id);
}

}  // namespace profiling
