// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_STREAM_SOCKET_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_STREAM_SOCKET_

#include "net/socket/stream_socket.h"
#include "content/browser/devtools/devtools_url_request_interceptor.h"
#include "base/time/time.h"
#include "base/timer/timer.h"

namespace content {

class DevtoolsStreamSocket : public net::StreamSocket {
 public:
  DevtoolsStreamSocket(std::unique_ptr<net::StreamSocket> base_socket,
    scoped_refptr<DevToolsURLRequestInterceptor::State> state);

  ~DevtoolsStreamSocket() override;

  int Read(net::IOBuffer* buf,
           int buf_len,
           const net::CompletionCallback& callback) override;
  int ReadIfReady(net::IOBuffer* buf,
                  int buf_len,
                  const net::CompletionCallback& callback) override;
  int Write(net::IOBuffer* buf,
            int buf_len,
            const net::CompletionCallback& callback) override;
  int SetReceiveBufferSize(int32_t size) override;
  int SetSendBufferSize(int32_t size) override;

  int Connect(const net::CompletionCallback& callback) override;
  void Disconnect() override;
  bool IsConnected() const override;
  bool IsConnectedAndIdle() const override;
  int GetPeerAddress(net::IPEndPoint* address) const override;
  int GetLocalAddress(net::IPEndPoint* address) const override;
  const net::NetLogWithSource& NetLog() const override;
  void SetSubresourceSpeculation() override;
  void SetOmniboxSpeculation() override;
  bool WasEverUsed() const override;
  void EnableTCPFastOpenIfSupported() override;
  bool WasAlpnNegotiated() const override;
  net::NextProto GetNegotiatedProtocol() const override;
  bool GetSSLInfo(net::SSLInfo* ssl_info) override;

  void GetConnectionAttempts(net::ConnectionAttempts* out) const override;
  void ClearConnectionAttempts() override;
  void AddConnectionAttempts(const net::ConnectionAttempts& attempts) override;
  int64_t GetTotalReceivedBytes() const override;

private:

  void OnTimerRead(net::IOBuffer* buf, int size, const net::CompletionCallback& callback);

	std::unique_ptr<net::StreamSocket> base_socket_;
  scoped_refptr<DevToolsURLRequestInterceptor::State> state_;
  base::OneShotTimer read_timer_;
  base::OneShotTimer write_timer_;

};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_STREAM_SOCKET_
