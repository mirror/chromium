// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/profiling/memlog_sender.h"

#include "base/command_line.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/profiling/memlog_allocator_shim.h"
#include "chrome/common/profiling/memlog_sender_pipe.h"
#include "chrome/common/profiling/memlog_stream.h"
#include "mojo/edk/embedder/embedder.h"
#include "mojo/edk/embedder/incoming_broker_client_invitation.h"
#include "mojo/edk/embedder/platform_channel_pair.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace profiling {

void InitMemlogSenderIfNecessary(const base::CommandLine& cmdline) {
  mojo::edk::Init();
  base::CommandLine::StringType pipe_id =
      cmdline.GetSwitchValueNative(switches::kMemlogPipe);
  LOG(ERROR) << "Child: Received: " << pipe_id;
  if (pipe_id.empty())
    return;  // No pipe, don't run.

  LOG(ERROR) << "Child: attemtping to connect to pipe";
  mojo::edk::ScopedPlatformHandle control_channel =
      mojo::edk::PlatformChannelPair::PassClientHandleFromParentProcessFromString(pipe_id);

  auto invitation = mojo::edk::IncomingBrokerClientInvitation::Accept(
      mojo::edk::ConnectionParams(
          mojo::edk::TransportProtocol::kLegacy,
          mojo::edk::PlatformChannelPair::PassClientHandleFromParentProcessFromString(pipe_id)));

  LOG(ERROR) << "child: Create ender pipe";
  static MemlogSenderPipe pipe(invitation->ExtractMessagePipe("profiling_control"));

  StreamHeader header;
  header.signature = kStreamSignature;

  LOG(ERROR) << "child: Send header";
  pipe.Send(&header, sizeof(StreamHeader));

//  InitAllocatorShim(&pipe);
}

}  // namespace profiling
