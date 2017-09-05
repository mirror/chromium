// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_transfer_to_client_message_handler.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/stl_util.h"
#include "remoting/base/compound_buffer.h"

namespace remoting {

FileTransferToClientMessageHandler::FileTransferToClientMessageHandler(
    const std::string& name,
    std::unique_ptr<protocol::MessagePipe> pipe,
    std::unique_ptr<FileProxyWrapper> file_proxy_wrapper)
    : protocol::NamedMessagePipeHandler(name, std::move(pipe)),
      file_proxy_wrapper_(std::move(file_proxy_wrapper)) {
  DCHECK(file_proxy_wrapper_);
}

FileTransferToClientMessageHandler::~FileTransferToClientMessageHandler() =
    default;

void FileTransferToClientMessageHandler::OnConnected() {
  // base::Unretained is safe here because |file_proxy_wrapper_| is owned by
  // this class, so the callback cannot be run after this class is destroyed.
  file_proxy_wrapper_->Init(
      base::BindOnce(&FileTransferToClientMessageHandler::StatusCallback,
                     base::Unretained(this)));
}

void FileTransferToClientMessageHandler::OnIncomingMessage(
    std::unique_ptr<CompoundBuffer> buffer) {
  if (!request_) {
    ParseNewRequest(std::move(buffer));
  }
}

void FileTransferToClientMessageHandler::OnDisconnecting() {
  FileProxyWrapper::State proxy_state = file_proxy_wrapper_->state();
  if (proxy_state != FileProxyWrapper::kClosed &&
      proxy_state != FileProxyWrapper::kFailed) {
    // Channel was closed earlier than expected, cancel the transfer.
    file_proxy_wrapper_->Cancel();
  }
}

void FileTransferToClientMessageHandler::StatusCallback(
    FileProxyWrapper::State state,
    base::Optional<protocol::FileTransferResponse_ErrorCode> error) {
  protocol::FileTransferToClientResponse response;
  if (error.has_value()) {
    // TODO(jarhar): Should we send an error message or just close the channel?
    DCHECK_EQ(state, FileProxyWrapper::kFailed);
    LOG(ERROR) << "jarhar@ " << __PRETTY_FUNCTION__
               << " error: " << error.value();
    Close();
  }
}

void FileTransferToClientMessageHandler::ParseNewRequest(
    std::unique_ptr<CompoundBuffer> buffer) {
  std::string message;
  message.resize(buffer->total_bytes());
  buffer->CopyTo(base::string_as_array(&message), message.size());

  request_ =
      base::MakeUnique<protocol::ClientInitiatedFileTransferToClientRequest>();
  if (!request_->ParseFromString(message)) {
    CancelAndSendError("Failed to parse request protobuf");
    return;
  }

  file_proxy_wrapper_->OpenFile(
      base::FilePath(request_->filepath()),
      base::Bind(&FileTransferToClientMessageHandler::OpenFileCallback,
                 base::Unretained(this)));
}

void FileTransferToClientMessageHandler::OpenFileCallback(int64_t filesize) {
  // now that the file is open, we can start the transfer
  filesize_ = static_cast<uint64_t>(filesize);
  protocol::FileTransferToClientRequest request;
  request.set_filesize(filesize);
  Send(request, base::Bind(&FileTransferToClientMessageHandler::SendChunk,
                           base::Unretained(this)));
}

void FileTransferToClientMessageHandler::SendChunk() {
  uint64_t bytes_to_read = 4096;
  if (total_bytes_read_ + bytes_to_read > filesize_) {
    // reading the last chunk
    bytes_to_read = filesize_ - total_bytes_read_;
  }
  file_proxy_wrapper_->ReadChunk(
      bytes_to_read,
      base::Bind(&FileTransferToClientMessageHandler::ReadChunkCallback,
                 base::Unretained(this)));
}

void FileTransferToClientMessageHandler::ReadChunkCallback(
    std::unique_ptr<std::vector<char>> chunk) {
  total_bytes_read_ += chunk->size();
  Send(*chunk,
       base::Bind(&FileTransferToClientMessageHandler::SentChunkCallback,
                  base::Unretained(this)));
}

void FileTransferToClientMessageHandler::SentChunkCallback() {
  // send another chunk if we can.
  if (total_bytes_read_ >= filesize_) {
    file_proxy_wrapper_->Close();
    // transfer is complete.
  } else {
    SendChunk();
  }
}

void FileTransferToClientMessageHandler::CancelAndSendError(
    const std::string& error) {
  LOG(ERROR) << error;
  file_proxy_wrapper_->Cancel();
  Close();
}

}  // namespace remoting
