// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/host/file_transfer_message_handler_factory.h"

#include "remoting/host/file_transfer_message_handler.h"

namespace remoting {

FileTransferMessageHandlerFactory::FileTransferMessageHandlerFactory(
    std::unique_ptr<FileTransferProxyFactory> file_proxy_factory)
    : file_proxy_factory_(std::move(file_proxy_factory)) {}

FileTransferMessageHandlerFactory::~FileTransferMessageHandlerFactory() =
    default;

void FileTransferMessageHandlerFactory::CreateDataChannelHandler(
    const std::string& channel_name,
    std::unique_ptr<protocol::MessagePipe> pipe) {
  // FileTransferMessageHandler manages its own lifetime and is tied to the
  // lifetime of |pipe|. Once |pipe| is closed, this instance will be cleaned
  // up.
  new FileTransferMessageHandler(channel_name, std::move(pipe),
                                 file_proxy_factory_->CreateProxy());
}

}  // namespace remoting
