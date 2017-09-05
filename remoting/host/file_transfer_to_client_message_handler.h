// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_FILE_TRANSFER_TO_CLIENT_MESSAGE_HANDLER_H_
#define REMOTING_HOST_FILE_TRANSFER_TO_CLIENT_MESSAGE_HANDLER_H_

#include <cstdint>
#include <memory>
#include <string>

#include "remoting/host/file_proxy_wrapper.h"
#include "remoting/proto/file_transfer.pb.h"
#include "remoting/protocol/named_message_pipe_handler.h"

namespace remoting {

constexpr char kFileTransferToClientDataChannelPrefix[] =
    "filetransfertoclient-";

class FileTransferToClientMessageHandler
    : public protocol::NamedMessagePipeHandler {
 public:
  FileTransferToClientMessageHandler(
      const std::string& name,
      std::unique_ptr<protocol::MessagePipe> pipe,
      std::unique_ptr<FileProxyWrapper> file_proxy);
  ~FileTransferToClientMessageHandler() override;

  // protocol::NamedMessagePipeHandler implementation.
  void OnConnected() override;
  void OnIncomingMessage(std::unique_ptr<CompoundBuffer> message) override;
  void OnDisconnecting() override;

 private:

  void StatusCallback(
      FileProxyWrapper::State state,
      base::Optional<protocol::FileTransferResponse_ErrorCode> error);
  void ParseNewRequest(std::unique_ptr<CompoundBuffer> buffer);
  void OpenFileCallback(int64_t filesize);
  void SendChunk();
  void ReadChunkCallback(std::unique_ptr<std::vector<char>> chunk);
  void SentChunkCallback();
  void CancelAndSendError(const std::string& error);

  std::unique_ptr<FileProxyWrapper> file_proxy_wrapper_;

  std::unique_ptr<protocol::ClientInitiatedFileTransferToClientRequest>
      request_;
  uint64_t total_bytes_read_ = 0;
  uint64_t filesize_ = 0;
};

}  // namespace remoting

#endif  // REMOTING_HOST_FILE_TRANSFER_TO_CLIENT_MESSAGE_HANDLER_H_
