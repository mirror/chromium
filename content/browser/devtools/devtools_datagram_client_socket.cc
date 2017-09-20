// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_datagram_client_socket.h"

namespace content {

DevtoolsDatagramClientSocket::DevtoolsDatagramClientSocket(
      std::unique_ptr<net::DatagramClientSocket> base_socket,
      scoped_refptr<DevToolsURLRequestInterceptor::State> state)
    : base_socket_(std::move(base_socket))
    , state_(state) {
  fprintf(stderr, "Devtools owns datastream socket.\n");
}

DevtoolsDatagramClientSocket::~DevtoolsDatagramClientSocket() {
}

int DevtoolsDatagramClientSocket::Read(net::IOBuffer* buf,
           int buf_len,
           const net::CompletionCallback& callback) {
  return base_socket_->Read(buf, buf_len, callback);
}

int DevtoolsDatagramClientSocket::ReadIfReady(net::IOBuffer* buf,
              int buf_len,
              const net::CompletionCallback& callback) {
  return base_socket_->ReadIfReady(buf, buf_len, callback);
}

int DevtoolsDatagramClientSocket::Write(net::IOBuffer* buf,
        int buf_len,
        const net::CompletionCallback& callback) {
  return base_socket_->Write(buf, buf_len, callback);
}

int DevtoolsDatagramClientSocket::SetReceiveBufferSize(int32_t size) {
  return base_socket_->SetReceiveBufferSize(size);
}
int DevtoolsDatagramClientSocket::SetSendBufferSize(int32_t size) {
  return base_socket_->SetSendBufferSize(size);
}

// DatagramClientSocket implementation.
int DevtoolsDatagramClientSocket::Connect(const net::IPEndPoint& address) {
  return base_socket_->Connect(address);
}
int DevtoolsDatagramClientSocket::ConnectUsingNetwork(net::NetworkChangeNotifier::NetworkHandle network,
                      const net::IPEndPoint& address) {
  return base_socket_->ConnectUsingNetwork(network, address);
}
int DevtoolsDatagramClientSocket::ConnectUsingDefaultNetwork(const net::IPEndPoint& address) {
  return base_socket_->ConnectUsingDefaultNetwork(address);
}
net::NetworkChangeNotifier::NetworkHandle DevtoolsDatagramClientSocket::GetBoundNetwork() const {
  return base_socket_->GetBoundNetwork();
}

void DevtoolsDatagramClientSocket::Close() {
  base_socket_->Close();
}
int DevtoolsDatagramClientSocket::GetPeerAddress(net::IPEndPoint* address) const {
  return base_socket_->GetPeerAddress(address);
}
int DevtoolsDatagramClientSocket::GetLocalAddress(net::IPEndPoint* address) const {
  return base_socket_->GetLocalAddress(address);
}
void DevtoolsDatagramClientSocket::UseNonBlockingIO() {
  base_socket_->UseNonBlockingIO();
}
int DevtoolsDatagramClientSocket::SetDoNotFragment() {
  return base_socket_->SetDoNotFragment();
}
const net::NetLogWithSource& DevtoolsDatagramClientSocket::NetLog() const {
  return base_socket_->NetLog();
}

} // namespace content
