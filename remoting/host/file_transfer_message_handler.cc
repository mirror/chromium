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
  file_proxy_wrapper_->Init(base::BindOnce(
      &FileTransferMessageHandler::DoneCallback, base::Unretained(this)));
}

void FileTransferMessageHandler::OnIncomingMessage(
    std::unique_ptr<CompoundBuffer> buffer) {
  FileProxyWrapper::State proxy_state = file_proxy_wrapper_->state();
  if (proxy_state == FileProxyWrapper::kClosing ||
      proxy_state == FileProxyWrapper::kClosed ||
      proxy_state == FileProxyWrapper::kFailed) {
    return;
  }

  if (request_) {
    // File transfer is already in progress, just pass the buffer to
    // FileProxyWrapper to be written.
    total_bytes_written_ += buffer->total_bytes();
    if (proxy_state == FileProxyWrapper::kFileCreated) {
      file_proxy_wrapper_->WriteChunk(std::move(buffer));
      if (total_bytes_written_ >= request_->filesize()) {
        file_proxy_wrapper_->Close();
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

    file_proxy_wrapper_->CreateFile(target_directory, request_->filename());
  } else {
    // Failed to parse the request protobuf.
    file_proxy_wrapper_->Cancel();
    protocol::FileTransferResponse response;
    response.set_error(
        protocol::FileTransferResponse_ErrorCode_UNEXPECTED_ERROR);
    Send(&response, base::Bind(&EmptyClosure));
  }
}

void FileTransferMessageHandler::OnDisconnecting() {
  FileProxyWrapper::State proxy_state = file_proxy_wrapper_->state();
  if (proxy_state != FileProxyWrapper::kClosed &&
      proxy_state != FileProxyWrapper::kFailed) {
    // Channel was closed earlier than expected, cancel the transfer.
    file_proxy_wrapper_->Cancel();
  }
}

void FileTransferMessageHandler::DoneCallback(
    std::unique_ptr<protocol::FileTransferResponse_ErrorCode> error) {
  protocol::FileTransferResponse response;
  if (error) {
    response.set_error(*error);
  } else {
    response.set_state(protocol::FileTransferResponse_TransferState_DONE);
    response.set_total_bytes_written(request_->filesize());
  }
  Send(&response, base::Bind(&EmptyClosure));
}

}  // namespace remoting
