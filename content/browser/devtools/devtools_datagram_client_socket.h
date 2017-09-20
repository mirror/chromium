// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_DATAGRAM_CLIENT_SOCKET_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_DATAGRAM_CLIENT_SOCKET_

#include "net/socket/datagram_client_socket.h"
#include "content/browser/devtools/devtools_url_request_interceptor.h"

namespace content {

class DevtoolsDatagramClientSocket : public net::DatagramClientSocket {
public:
  DevtoolsDatagramClientSocket(
      std::unique_ptr<net::DatagramClientSocket> base_socket,
      scoped_refptr<DevToolsURLRequestInterceptor::State> state);

  ~DevtoolsDatagramClientSocket() override;

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

  // DatagramClientSocket implementation.
  int Connect(const net::IPEndPoint& address) override;
  int ConnectUsingNetwork(net::NetworkChangeNotifier::NetworkHandle network,
                          const net::IPEndPoint& address) override;
  int ConnectUsingDefaultNetwork(const net::IPEndPoint& address) override;
  net::NetworkChangeNotifier::NetworkHandle GetBoundNetwork() const override;

  void Close() override;
  int GetPeerAddress(net::IPEndPoint* address) const override;
  int GetLocalAddress(net::IPEndPoint* address) const override;
  void UseNonBlockingIO() override;
  int SetDoNotFragment() override;
  const net::NetLogWithSource& NetLog() const override;

scoped_refptr<DevToolsURLRequestInterceptor::State> state() {
  return state_;
}

private:
  std::unique_ptr<net::DatagramClientSocket> base_socket_;
  scoped_refptr<DevToolsURLRequestInterceptor::State> state_;
  //base::OneShotTimer timer_;

};

} // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_DATAGRAM_CLIENT_SOCKET_
