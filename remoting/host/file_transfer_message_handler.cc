// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_transfer_message_handler.h"

namespace remoting {

FileTransferMessageHandler::FileTransferMessageHandler(
    std::unique_ptr<protocol::MessagePipe> pipe)
    : pipe_(std::move(pipe)) {
  DCHECK(pipe_);
  pipe_->Start(this);
}

FileTransferMessageHandler::~FileTransferMessageHandler() = default;

void FileTransferMessageHandler::OnMessagePipeOpen() {
  // TODO(jarhar): Implement open logic.
}

void FileTransferMessageHandler::OnMessageReceived(
    std::unique_ptr<CompoundBuffer> message) {
  // TODO(jarhar): Implement message received logic.
}

void FileTransferMessageHandler::OnMessagePipeClosed() {
  // TODO(jarhar): Implement close logic.

  delete this;
}

}  // namespace remoting
