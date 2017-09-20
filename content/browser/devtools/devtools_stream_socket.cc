// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_stream_socket.h"
#include "base/rand_util.h"
#include "net/base/net_errors.h"

namespace content {

int kPacketSize = 1500;

DevtoolsStreamSocket::DevtoolsStreamSocket(std::unique_ptr<net::StreamSocket> base_socket,
  scoped_refptr<DevToolsURLRequestInterceptor::State> state)
  : base_socket_(std::move(base_socket))
  , state_(state) {
    fprintf(stderr, "Devtools owns stream socket.\n");
}

DevtoolsStreamSocket::~DevtoolsStreamSocket() {
}

void DevtoolsStreamSocket::OnTimerRead(net::IOBuffer* buf, int size,
    const net::CompletionCallback& callback) {
  int rv = base_socket_->Read(buf, size, callback);
  fprintf(stderr, "Read Timer %d\n", rv);
  if (rv != net::ERR_IO_PENDING)
    callback.Run(rv);
}

int DevtoolsStreamSocket::Read(net::IOBuffer* buf,
         int buf_len,
         const net::CompletionCallback& callback) {
  // int64_t us_tick_length = (10000000000L * kPacketSize) / state_->DownloadSpeed();
  // read_timer_.Start(
  //     FROM_HERE, base::TimeDelta::FromMicroseconds(us_tick_length),
  //     base::Bind(&DevtoolsStreamSocket::OnTimerRead, base::Unretained(this),
  //       buf, std::min(buf_len, kPacketSize), callback));
  return net::ERR_IO_PENDING;
}
int DevtoolsStreamSocket::ReadIfReady(net::IOBuffer* buf,
                int buf_len,
                const net::CompletionCallback& callback) {
  //if (base::RandInt(0, 500) == 0) {
    return base_socket_->ReadIfReady(buf, buf_len, callback);
  //}
  //return net::ERR_IO_PENDING;
}
int DevtoolsStreamSocket::Write(net::IOBuffer* buf,
          int buf_len,
          const net::CompletionCallback& callback) {
  return base_socket_->Write(buf, buf_len, callback);
}
int DevtoolsStreamSocket::SetReceiveBufferSize(int32_t size) {
  return base_socket_->SetReceiveBufferSize(size);
}
int DevtoolsStreamSocket::SetSendBufferSize(int32_t size) {
  return base_socket_->SetSendBufferSize(size);
}

int DevtoolsStreamSocket::Connect(const net::CompletionCallback& callback) {
  return base_socket_->Connect(callback);
}
void DevtoolsStreamSocket::Disconnect() {
  base_socket_->Disconnect();
}
bool DevtoolsStreamSocket::IsConnected() const {
  return base_socket_->IsConnected();
}
bool DevtoolsStreamSocket::IsConnectedAndIdle() const {
  return base_socket_->IsConnectedAndIdle();
}
int DevtoolsStreamSocket::GetPeerAddress(net::IPEndPoint* address) const {
  return base_socket_->GetPeerAddress(address);
}
int DevtoolsStreamSocket::GetLocalAddress(net::IPEndPoint* address) const {
  return base_socket_->GetLocalAddress(address);
}
const net::NetLogWithSource& DevtoolsStreamSocket::NetLog() const {
  return base_socket_->NetLog();
}
void DevtoolsStreamSocket::SetSubresourceSpeculation() {
  base_socket_->SetSubresourceSpeculation();
}
void DevtoolsStreamSocket::SetOmniboxSpeculation() {
  base_socket_->SetOmniboxSpeculation();
}
bool DevtoolsStreamSocket::WasEverUsed() const {
  return base_socket_->WasEverUsed();
}
void DevtoolsStreamSocket::EnableTCPFastOpenIfSupported() {
  base_socket_->EnableTCPFastOpenIfSupported();
}
bool DevtoolsStreamSocket::WasAlpnNegotiated() const {
  return base_socket_->WasAlpnNegotiated();
}
net::NextProto DevtoolsStreamSocket::GetNegotiatedProtocol() const {
  return base_socket_->GetNegotiatedProtocol();
}
bool DevtoolsStreamSocket::GetSSLInfo(net::SSLInfo* ssl_info) {
  return base_socket_->GetSSLInfo(ssl_info);
}

void DevtoolsStreamSocket::GetConnectionAttempts(net::ConnectionAttempts* out) const {
  base_socket_->GetConnectionAttempts(out);
}
void DevtoolsStreamSocket::ClearConnectionAttempts() {
  base_socket_->ClearConnectionAttempts();
}
void DevtoolsStreamSocket::AddConnectionAttempts(const net::ConnectionAttempts& attempts) {
  base_socket_->AddConnectionAttempts(attempts);
}
int64_t DevtoolsStreamSocket::GetTotalReceivedBytes() const {
  return base_socket_->GetTotalReceivedBytes();
}

} // namespace content

