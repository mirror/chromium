// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/external_process_connection_dispatcher.h"

#include <poll.h>

#include "base/bind.h"
#include "base/task_scheduler/post_task.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/incoming_broker_client_invitation.h"
#include "mojo/edk/embedder/named_platform_handle.h"
#include "mojo/edk/embedder/named_platform_handle_utils.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
// #include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/platform_channel_utils_posix.h"
// #include "mojo/edk/embedder/scoped_ipc_support.h"

namespace {

void JoinAndReleaseThread(std::unique_ptr<base::Thread> thread) {
  if (!thread)
    return;

  LOG(ERROR) << "Shutting down thread";
  thread.reset();
}

static const base::FilePath::CharType kSocketPath[] =
    "/tmp/chromium_external_process.sock";

}  // anonymous namespace

namespace content {

ExternalProcessConnectionDispatcher::ExternalProcessConnectionDispatcher()
    : owner_task_runner_(base::ThreadTaskRunnerHandle::Get()), binding_(this) {}

ExternalProcessConnectionDispatcher::~ExternalProcessConnectionDispatcher() {
  DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());
}

void ExternalProcessConnectionDispatcher::
    StartListeningForIncomingConnection() {
  DCHECK(!thread_);

  thread_ =
      std::make_unique<base::Thread>("ExternalProcessConnectionDispatcher");
  thread_->StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));

  thread_->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&ExternalProcessConnectionDispatcher::
                                    ListenAndBlockForIncomingConnection,
                                base::Unretained(this)));
}

void ExternalProcessConnectionDispatcher::ConnectToDeviceFactory(
    video_capture::mojom::DeviceFactoryRequest request) {
  LOG(ERROR) << "ConnectToDeviceFactory call received";
}

void ExternalProcessConnectionDispatcher::SetShutdownDelayInSeconds(
    float seconds) {}

void ExternalProcessConnectionDispatcher::
    ListenAndBlockForIncomingConnection() {
  base::FilePath socket_path(kSocketPath);
  mojo::edk::NamedPlatformHandle named_handle(socket_path.value());
  mojo::edk::ScopedPlatformHandle platform_handle =
      mojo::edk::CreateServerHandle(
          mojo::edk::NamedPlatformHandle(socket_path.value()));
  if (!platform_handle.is_valid()) {
    LOG(ERROR) << "Failed to create the socket file: " << kSocketPath;
    return;
  }

  // Wait for an external process to connect.
  // TODO: Provide a cancel signal that we can use during shutdown.
  LOG(ERROR) << "Waiting for external process to connect ...";
  struct pollfd fds[1] = {
      {platform_handle.get().handle, POLLIN, 0},
  };
  if (HANDLE_EINTR(poll(fds, arraysize(fds), -1)) <= 0) {
    PLOG(ERROR) << "poll()";
    return;
  }

  LOG(WARNING) << "Accepting connection";
  mojo::edk::ScopedPlatformHandle accepted_fd;
  if (!mojo::edk::ServerAcceptConnection(platform_handle, &accepted_fd,
                                         false)) {
    LOG(ERROR) << "ServerAcceptConnection returned false";
    return;
  }
  if (!accepted_fd.is_valid()) {
    LOG(ERROR) << "accepted_fd is not valid";
    return;
  }

  owner_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&ExternalProcessConnectionDispatcher::
                         SendMojoInvitationToExternalProcess,
                     // use of |Unretained(this)| is safe because |this| gets
                     // destroyed on |owner_task_runner_|.
                     base::Unretained(this), base::Passed(&accepted_fd)));
}

void ExternalProcessConnectionDispatcher::SendMojoInvitationToExternalProcess(
    mojo::edk::ScopedPlatformHandle handle) {
  DCHECK(owner_task_runner_->RunsTasksInCurrentSequence());

  mojo::edk::OutgoingBrokerClientInvitation invitation;

  mojo::ScopedMessagePipeHandle message_pipe =
      invitation.AttachMessagePipe("pipe");

  LOG(ERROR) << "Sending invitation";
  const base::ProcessHandle kArbitraryUnusedProcessHandle = 0u;
  invitation.Send(
      kArbitraryUnusedProcessHandle,
      mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                  std::move(handle)));

  LOG(ERROR) << "Binding message pipe to |this|";
  binding_.Bind(video_capture::mojom::DeviceFactoryProviderRequest(
      std::move(message_pipe)));
  binding_.set_connection_error_handler(base::BindOnce(
      &ExternalProcessConnectionDispatcher::OnExternalProcessDisconnected,
      base::Unretained(this)));

  thread_->DetachFromSequence();
  base::PostTaskWithTraits(
      FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
      base::BindOnce(&JoinAndReleaseThread, base::Passed(&thread_)));
}

void ExternalProcessConnectionDispatcher::OnExternalProcessDisconnected() {
  LOG(ERROR) << "External process disconnected";
  binding_.Close();
  StartListeningForIncomingConnection();
}

}  // namespace content
