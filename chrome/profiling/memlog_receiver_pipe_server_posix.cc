// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/memlog_receiver_pipe_server_posix.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/profiling/memlog_stream.h"

#include "mojo/edk/embedder/platform_channel_pair.h"

namespace profiling {

MemlogReceiverPipeServer::MemlogReceiverPipeServer(
    base::TaskRunner* io_runner,
    const std::string& pipe_id,
    NewConnectionCallback on_new_conn)
    : io_runner_(io_runner),
      pipe_id_(pipe_id),
      on_new_connection_(on_new_conn) {}

MemlogReceiverPipeServer::~MemlogReceiverPipeServer() {}

void MemlogReceiverPipeServer::Start() {
  // Connect to the pipe_id_ on IO thread.
  /*
  mojo::edk::SetParentPipeHandle(
      mojo::edk::PlatformChannelPair::PassClientHandleFromParentProcess(
          command_line));
  mojo::edk::ScopedPlatformHandle parent_pipe =
      mojo::edk::PlatformChannelPair::PassClientHandleFromParentProcess(
          *command_line);

  std::string token = command_line.GetSwitchValueASCII("primordial-pipe");
  mojo::ScopedMessagePipeHandle pipe = mojo::edk::CreateChildMessagePipe(token);
  */
}

}  // namespace profiling
