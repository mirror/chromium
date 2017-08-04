// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_FILE_TRANSFER_MESSAGE_HANDLER_H_
#define REMOTING_HOST_FILE_TRANSFER_MESSAGE_HANDLER_H_

#include "remoting/protocol/message_pipe.h"

namespace remoting {

class FileTransferMessageHandler : public protocol::MessagePipe::EventHandler {
 public:
  FileTransferMessageHandler(std::unique_ptr<protocol::MessagePipe> pipe);
  ~FileTransferMessageHandler() override;

  // protocol::MessageHandler implementation.
  void OnMessagePipeOpen() override;
  void OnMessageReceived(std::unique_ptr<CompoundBuffer> message) override;
  void OnMessagePipeClosed() override;

 private:
  std::unique_ptr<protocol::MessagePipe> pipe_;
};

}  // namespace remoting

#endif  // REMOTING_HOST_FILE_TRANSFER_MESSAGE_HANDLER_H_
