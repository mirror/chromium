// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/profiling/memlog_sender_pipe_posix.h"

#include <unistd.h>

#include "base/logging.h"
#include "base/posix/eintr_wrapper.h"
#include "chrome/common/profiling/memlog_stream.h"
#include "mojo/public/cpp/system/message.h"
#include "mojo/public/cpp/system/message_pipe.h"

namespace profiling {
namespace {

struct RawMessage {
  size_t size;
  const void* data;
};

void GetSerializedSize(uintptr_t context,
                       size_t* num_bytes,
                       size_t* num_handles) {
  RawMessage* raw_message = reinterpret_cast<RawMessage*>(context);
  *num_bytes = raw_message->size;
  *num_handles = 0;
}

void SerializeHandles(uintptr_t context, MojoHandle* handles) {
}

void SerializePayload(uintptr_t context, void* buffer) {
  LOG(ERROR) << "child: serializing.";
  RawMessage* raw_message = reinterpret_cast<RawMessage*>(context);
  memcpy(buffer, raw_message->data, raw_message->size);
}

void Destroy(uintptr_t context) {
}

MojoMessageOperationThunks g_thunks = {
  sizeof(MojoMessageOperationThunks),
  &GetSerializedSize,
  &SerializeHandles,
  &SerializePayload,
  &Destroy,
};
}  // namespace

MemlogSenderPipe::MemlogSenderPipe(mojo::ScopedMessagePipeHandle control_pipe)
    : control_pipe_(std::move(control_pipe)) {}

MemlogSenderPipe::~MemlogSenderPipe() {
}

bool MemlogSenderPipe::Send(const void* data, size_t sz) {
  RawMessage raw_message = {sz, data};
  mojo::ScopedMessageHandle message;

  LOG(ERROR) << "child: Sending data.";
  MojoResult result = mojo::CreateMessage(
      reinterpret_cast<uintptr_t>(&raw_message), &g_thunks, &message);
  if (result != MOJO_RESULT_OK) {
    return false;
  }

  result = mojo::WriteMessageNew(control_pipe_.get(), std::move(message),
                                 MOJO_WRITE_MESSAGE_FLAG_NONE);
  return result == MOJO_RESULT_OK;
}

}  // namespace profiling
