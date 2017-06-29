// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/profiling/memlog_receiver_pipe_server_posix.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/common/profiling/memlog_stream.h"

#include "mojo/edk/embedder/platform_channel_pair.h"

namespace profiling {

MemlogReceiverPipeServer::MemlogReceiverPipeServer(
    base::TaskRunner* io_runner,
    mojo::ScopedMessagePipeHandle control_pipe,
    NewConnectionCallback on_new_conn)
    : io_runner_(io_runner),
      control_pipe_(std::move(control_pipe)),
      on_new_connection_(on_new_conn) {}

MemlogReceiverPipeServer::~MemlogReceiverPipeServer() {}

void MemlogReceiverPipeServer::Start() {
  io_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MemlogReceiverPipeServer::StartWatchOnIO, this));
}
void MemlogReceiverPipeServer::StartWatchOnIO(void) {
  watcher_.reset(new mojo::SimpleWatcher(FROM_HERE,
               mojo::SimpleWatcher::ArmingPolicy::AUTOMATIC));
  watcher_->Watch(control_pipe_.get(),
                  MOJO_HANDLE_SIGNAL_READABLE,
                  base::Bind(&MemlogReceiverPipeServer::OnReadable, this));
  watcher_->ArmOrNotify();
}

void MemlogReceiverPipeServer::OnReadable(MojoResult result) {
  LOG(ERROR) << "parent: got signaled: " << result;
  while (result == MOJO_RESULT_OK) {
    mojo::ScopedMessageHandle message;
    uint32_t num_bytes;
    result = mojo::ReadMessageNew(control_pipe_.get(), &message, MOJO_READ_MESSAGE_FLAG_NONE);
    void* buffer;
    GetSerializedMessageContents(message.get(), &buffer, &num_bytes, nullptr,
                                 MOJO_GET_SERIALIZED_MESSAGE_CONTENTS_FLAG_NONE);
    LOG(ERROR) << "parent: desserilized.";
    char* foo = reinterpret_cast<char*>(buffer);
    LOG(ERROR) << foo;
  }
}

}  // namespace profiling
