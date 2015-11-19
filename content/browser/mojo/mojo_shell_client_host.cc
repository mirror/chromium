// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/thread_task_runner_handle.h"
#include "content/browser/mojo/mojo_shell_client_host.h"
#include "content/common/mojo/mojo_messages.h"
#include "content/public/common/mojo_shell_connection.h"
#include "ipc/ipc_sender.h"
#include "mojo/application/public/cpp/application_impl.h"
#include "mojo/converters/network/network_type_converters.h"
#include "mojo/shell/application_manager.mojom.h"
#include "third_party/mojo/src/mojo/edk/embedder/embedder.h"
#include "third_party/mojo/src/mojo/edk/embedder/platform_channel_pair.h"
#include "third_party/mojo/src/mojo/edk/embedder/scoped_platform_handle.h"

namespace content {
namespace {
void DidCreateChannel(mojo::embedder::ChannelInfo* info) {}

base::PlatformFile PlatformFileFromScopedPlatformHandle(
    mojo::embedder::ScopedPlatformHandle handle) {
#if defined(OS_POSIX)
  return handle.release().fd;
#elif defined(OS_WIN)
  return handle.release().handle;
#endif
}

}  // namespace

void RegisterChildWithExternalShell(int child_process_id,
                                    base::ProcessHandle process_handle,
                                    IPC::Sender* sender) {
  // Some process types get created before the main message loop.
  if (!MojoShellConnection::Get())
    return;

  // Create the channel to be shared with the target process.
  mojo::embedder::HandlePassingInformation handle_passing_info;
  mojo::embedder::PlatformChannelPair platform_channel_pair;

  // Give one end to the shell so that it can create an instance.
  mojo::embedder::ScopedPlatformHandle platform_channel =
      platform_channel_pair.PassServerHandle();
  mojo::ScopedMessagePipeHandle handle(mojo::embedder::CreateChannel(
      platform_channel.Pass(), base::Bind(&DidCreateChannel),
      base::ThreadTaskRunnerHandle::Get()));
  mojo::shell::mojom::ApplicationManagerPtr application_manager;
  MojoShellConnection::Get()->GetApplication()->ConnectToService(
      mojo::URLRequest::From(std::string("mojo:shell")),
      &application_manager);
  // The content of the URL/qualifier we pass is actually meaningless, it's only
  // important that they're unique per process.
  // TODO(beng): We need to specify a restrictive CapabilityFilter here that
  //             matches the needs of the target process. Figure out where that
  //             specification is best determined (not here, this is a common
  //             chokepoint for all process types) and how to wire it through.
  //             http://crbug.com/555393
  application_manager->CreateInstanceForHandle(
      mojo::ScopedHandle(mojo::Handle(handle.release().value())),
      "exe:chrome_renderer",  // See above about how this string is meaningless.
      base::IntToString(child_process_id));

  // Send the other end to the child via Chrome IPC.
  base::PlatformFile client_file = PlatformFileFromScopedPlatformHandle(
      platform_channel_pair.PassClientHandle());
  sender->Send(new MojoMsg_BindExternalMojoShellHandle(
      IPC::GetFileHandleForProcess(client_file, process_handle, true)));
}

}  // namespace content
