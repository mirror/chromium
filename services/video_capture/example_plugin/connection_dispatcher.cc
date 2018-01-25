// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/video_capture/example_plugin/connection_dispatcher.h"

#include <poll.h>

#include "base/bind.h"
#include "base/threading/platform_thread.h"
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
    "/tmp/chromium_external_process.sock";

}  // anonymous namespace

namespace video_capture {
namespace example_plugin {

ConnectionDispatcher::ConnectionDispatcher(
    ConnectionEstablishedCallback connection_established_cb)
    : connection_established_cb_(std::move(connection_established_cb)) {}

ConnectionDispatcher::~ConnectionDispatcher() = default;

void ConnectionDispatcher::RunConnectionAttemptLoop() {
  mojo::edk::Init();
  ipc_thread_ = std::make_unique<base::Thread>("IPC");
  ipc_thread_->StartWithOptions(
      base::Thread::Options(base::MessageLoop::TYPE_IO, 0));
  ipc_support_ = std::make_unique<mojo::edk::ScopedIPCSupport>(
      ipc_thread_->task_runner(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  base::FilePath socket_path(kSocketPath);
  bool connected = false;
  // TODO: Instead of polling, we could watch for a socket to get created at
  // |socket_path|
  while (!connected) {
    if (!base::PathExists(socket_path)) {
      LOG(ERROR) << "Socket file for PluginConnectionDispatcher not present";
      base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(1));
      continue;
    }

    mojo::edk::ScopedPlatformHandle platform_handle =
        mojo::edk::CreateClientHandle(
            mojo::edk::NamedPlatformHandle(socket_path.value()));
    if (!platform_handle.is_valid()) {
      LOG(ERROR) << "Failed to create client platform handle from path: "
                 << kSocketPath;
      base::PlatformThread::Sleep(base::TimeDelta::FromSeconds(1));
      continue;
    }

    mojo::edk::OutgoingBrokerClientInvitation invitation;

    mojo::ScopedMessagePipeHandle message_pipe =
        invitation.AttachMessagePipe("pipe");

    const base::ProcessHandle kArbitraryUnusedProcessHandle = 0u;
    invitation.Send(
        kArbitraryUnusedProcessHandle,
        mojo::edk::ConnectionParams(mojo::edk::TransportProtocol::kLegacy,
                                    std::move(platform_handle)));

    provider_.Bind(
        mojom::DeviceFactoryProviderPtrInfo(std::move(message_pipe), 0u));
    // provider_.set_connection_error_handler(base::BindOnce(
    //     &ConnectionDispatcher::OnConnectionLost, base::Unretained(this)));

    mojom::DeviceFactoryPtr device_factory;
    provider_->ConnectToDeviceFactory(mojo::MakeRequest(&device_factory));
    connection_established_cb_.Run(std::move(device_factory));

    connected = true;
  }
}

// void ConnectionDispatcher::OnConnectionLost() {
//   provider_.reset();
// }

}  // namespace example_plugin
}  // namespace video_capture
