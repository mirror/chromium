// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_transfer_message_handler.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "remoting/base/compound_buffer.h"

namespace {

void EmptyClosure() {}

}  // namespace

namespace remoting {

FileTransferMessageHandler::FileTransferMessageHandler(
    const std::string& name,
    std::unique_ptr<protocol::MessagePipe> pipe,
    std::unique_ptr<FileProxyWrapper> file_proxy_wrapper)
    : protocol::NamedMessagePipeHandler(name, std::move(pipe)),
      file_proxy_wrapper_(std::move(file_proxy_wrapper)),
      total_bytes_written_(0) {}

FileTransferMessageHandler::~FileTransferMessageHandler() = default;

void FileTransferMessageHandler::OnConnected() {
  file_proxy_wrapper_->Init(base::Bind(
      &FileTransferMessageHandler::ErrorCallback, base::Unretained(this)));
}

void FileTransferMessageHandler::OnIncomingMessage(
    std::unique_ptr<CompoundBuffer> buffer) {
  FileProxyWrapper::State proxy_state = file_proxy_wrapper_->state();
  if (proxy_state == FileProxyWrapper::CLOSING ||
      proxy_state == FileProxyWrapper::COMPLETED ||
      proxy_state == FileProxyWrapper::CANCELLED) {
    return;
  }

  if (request_) {
    // File transfer is already in progress, just pass the buffer to
    // FileProxyWrapper to be written.
    total_bytes_written_ += buffer->total_bytes();
    if (proxy_state == FileProxyWrapper::FILE_CREATED) {
      file_proxy_wrapper_->WriteChunk(std::move(buffer));
      if (total_bytes_written_ >= request_->filesize()) {
        file_proxy_wrapper_->Close(
            base::Bind(&FileTransferMessageHandler::CloseCallback,
                       base::Unretained(this)));
      }
    } else {
      LOG(ERROR) << "File transfer received data while file proxy wrapper is "
                    "in the wrong state: "
                 << proxy_state;
    }

    if (total_bytes_written_ > request_->filesize()) {
      LOG(ERROR) << "File transfer received " << total_bytes_written_
                 << " bytes, but request said there would only be "
                 << request_->filesize() << " bytes.";
    }
    return;
  }

  std::string message;
  message.resize(buffer->total_bytes());
  buffer->CopyTo(&(message[0]), message.size());

  request_ = base::WrapUnique(new protocol::FileTransferRequest());
  if (request_->ParseFromString(message)) {
    base::FilePath target_directory;
    // TODO(jarhar): Implement platform specific target directory selector.
    PathService::Get(base::DIR_USER_DESKTOP, &target_directory);

    file_proxy_wrapper_->CreateFile(
        target_directory, request_->filename(),
        base::Bind(&FileTransferMessageHandler::CreateCallback,
                   base::Unretained(this)));
  } else {
    // Failed to parse the request protobuf.
    file_proxy_wrapper_->Cancel();
    protocol::FileTransferResponse response;
    // TODO(jarhar): Which ErrorCode should be used here?
    response.set_error(protocol::FileTransferResponse_ErrorCode_FILE_IO_ERROR);
    Send(&response, base::Bind(&EmptyClosure));
  }
}

void FileTransferMessageHandler::OnDisconnecting() {
  FileProxyWrapper::State proxy_state = file_proxy_wrapper_->state();
  if (proxy_state != FileProxyWrapper::COMPLETED &&
      proxy_state != FileProxyWrapper::CANCELLED) {
    // Channel was closed earlier than expected, cancel the transfer.
    file_proxy_wrapper_->Cancel();
  }
}

void FileTransferMessageHandler::ErrorCallback(
    protocol::FileTransferResponse_ErrorCode error) {
  protocol::FileTransferResponse response;
  response.set_error(error);
  Send(&response, base::Bind(&EmptyClosure));
}

void FileTransferMessageHandler::CreateCallback() {
  protocol::FileTransferResponse response;
  response.set_state(protocol::FileTransferResponse_TransferState_READY);
  response.set_total_bytes_written(0);
  Send(&response, base::Bind(&EmptyClosure));
}

void FileTransferMessageHandler::CloseCallback() {
  protocol::FileTransferResponse response;
  response.set_state(protocol::FileTransferResponse_TransferState_DONE);
  response.set_total_bytes_written(request_->filesize());
  Send(&response, base::Bind(&EmptyClosure));
}

}  // namespace remoting
