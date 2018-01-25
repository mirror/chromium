// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/external_process_connection_dispatcher.h"

#include <poll.h>

#include "base/bind.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/incoming_broker_client_invitation.h"
#include "mojo/edk/embedder/named_platform_handle.h"
#include "mojo/edk/embedder/named_platform_handle_utils.h"
// #include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
// #include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/platform_channel_utils_posix.h"
// #include "mojo/edk/embedder/scoped_ipc_support.h"

namespace {

static const base::FilePath::CharType kSocketPath[] =
    "/tmp/chromium_external_process.sock";

}  // anonymous namespace

namespace content {

ExternalProcessConnectionDispatcher::ExternalProcessConnectionDispatcher() =
    default;
ExternalProcessConnectionDispatcher::~ExternalProcessConnectionDispatcher() =
    default;

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

  auto invitation = mojo::edk::IncomingBrokerClientInvitation::Accept(
      mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                  std::move(accepted_fd)));

  mojo::ScopedMessagePipeHandle message_pipe =
      invitation->ExtractMessagePipe("pipe");

  binding_set_.AddBinding(this,
                          video_capture::mojom::DeviceFactoryProviderRequest(
                              std::move(message_pipe)));
}

void ExternalProcessConnectionDispatcher::ConnectToDeviceFactory(
    video_capture::mojom::DeviceFactoryRequest request) {
  LOG(ERROR) << "ConnectToDeviceFactory call received";
}

void ExternalProcessConnectionDispatcher::SetShutdownDelayInSeconds(
    float seconds) {}

}  // namespace content
