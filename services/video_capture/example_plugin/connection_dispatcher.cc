// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/example_plugin/connection_dispatcher.h"

#include <poll.h>

#include "base/bind.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/incoming_broker_client_invitation.h"
#include "mojo/edk/embedder/named_platform_handle.h"
#include "mojo/edk/embedder/named_platform_handle_utils.h"
#include "mojo/edk/embedder/outgoing_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/platform_channel_utils_posix.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"

namespace {

static const base::FilePath::CharType kSocketPath[] =
    "/tmp/video_capture_plugin.sock";

}  // anonymous namespace

namespace video_capture {
namespace example_plugin {

ConnectionDispatcher::ConnectionDispatcher(
    DeviceFactoryRegisteredCallback device_factory_registered_cb)
    : device_factory_registered_cb_(std::move(device_factory_registered_cb)) {}

ConnectionDispatcher::~ConnectionDispatcher() = default;

void ConnectionDispatcher::StartListeningForIncomingConnection() {
  mojo::edk::Init();
  ipc_thread_ = std::make_unique<base::Thread>("IPC");
  ipc_thread_->StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
  ipc_support_ = std::make_unique<mojo::edk::ScopedIPCSupport>(
      ipc_thread_->task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  base::FilePath socket_path(kSocketPath);
  mojo::edk::NamedPlatformHandle named_handle(socket_path.value());
  mojo::edk::ScopedPlatformHandle platform_handle =
      mojo::edk::CreateServerHandle(
          mojo::edk::NamedPlatformHandle(socket_path.value()));
  if (!platform_handle.is_valid()) {
    LOG(ERROR) << "Failed to create the socket file: " << kSocketPath;
    return;
  }

  // Wait for a client (Chromium instance) to connect.
  LOG(WARNING) << "Waiting for Chromium to connect ...";
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

  binding_set_.AddBinding(
      this, video_capture::mojom::PluginConnectionDispatcherRequest(
                std::move(message_pipe)));
}

void ConnectionDispatcher::RegisterDeviceFactory(
    video_capture::mojom::DeviceFactoryPtr device_factory) {
  device_factory_registered_cb_.Run(std::move(device_factory));
}

}  // namespace example_plugin
}  // namespace video_capture
