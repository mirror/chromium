// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/profiling/memlog_sender.h"

#include "base/command_line.h"
#include "base/threading/thread.h"
#include "base/message_loop/message_loop.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/profiling/memlog_allocator_shim.h"
#include "chrome/common/profiling/memlog_sender_pipe.h"
#include "chrome/common/profiling/memlog_stream.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/incoming_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/edk/embedder/scoped_ipc_support.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace profiling {

void InitMemlogSenderIfNecessary(const base::CommandLine& cmdline) {
  base::CommandLine::StringType pipe_id =
      cmdline.GetSwitchValueNative(switches::kMemlogPipe);
  LOG(ERROR) << "Child: Received: " << pipe_id;
  if (pipe_id.empty())
    return;  // No pipe, don't run.
  mojo::edk::Init();
  base::Thread::Options options;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
  base::Thread io_thread("hack io thread");
  io_thread.StartWithOptions(options);
  mojo::edk::ScopedIPCSupport ipc_support(
      io_thread.task_runner().get(),
      mojo::edk::ScopedIPCSupport::ShutdownPolicy::CLEAN);

  LOG(ERROR) << "Child: attemtping to connect to pipe";
  mojo::edk::ScopedPlatformHandle control_channel =
      mojo::edk::PlatformChannelPair::PassClientHandleFromParentProcessFromString(pipe_id);

  LOG(ERROR) << "Parsed Platofrm Handle";
  auto invitation = mojo::edk::IncomingBrokerClientInvitation::Accept(
      mojo::edk::ConnectionParams(
          mojo::edk::TransportProtocol::kLegacy,
          std::move(control_channel)));

  LOG(ERROR) << "child: Create message pipe";
  static MemlogSenderPipe pipe(invitation->ExtractMessagePipe("profiling_control"));

  StreamHeader header;
  header.signature = kStreamSignature;

  LOG(ERROR) << "child: Send header";
  pipe.Send(&header, sizeof(StreamHeader));
  pipe.Send(&header, sizeof(StreamHeader));
  pipe.Send(&header, sizeof(StreamHeader));
  pipe.Send(&header, sizeof(StreamHeader));
  pipe.Send(&header, sizeof(StreamHeader));
  pipe.Send(&header, sizeof(StreamHeader));

//  InitAllocatorShim(&pipe);
}

}  // namespace profiling
