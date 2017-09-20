// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_CLINET_SOCKET_FACTORY_
#define CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_CLINET_SOCKET_FACTORY_

#include "net/socket/client_socket_factory.h"
#include "content/browser/devtools/devtools_url_request_interceptor.h"

namespace content {

class DevtoolsClientSocketFactory : public net::ClientSocketFactory {
public:
  DevtoolsClientSocketFactory(
      scoped_refptr<DevToolsURLRequestInterceptor::State> state,
      net::ClientSocketFactory* base_client_factory);

  ~DevtoolsClientSocketFactory() override;

  std::unique_ptr<net::DatagramClientSocket> CreateDatagramClientSocket(
      net::DatagramSocket::BindType bind_type,
      const net::RandIntCallback& rand_int_cb,
      net::NetLog* net_log,
      const net::NetLogSource& source) override;

  std::unique_ptr<net::StreamSocket> CreateTransportClientSocket(
      const net::AddressList& addresses,
      std::unique_ptr<net::SocketPerformanceWatcher> socket_performance_watcher,
      net::NetLog* net_log,
      const net::NetLogSource& source) override;

  std::unique_ptr<net::SSLClientSocket> CreateSSLClientSocket(
      std::unique_ptr<net::ClientSocketHandle> transport_socket,
      const net::HostPortPair& host_and_port,
      const net::SSLConfig& ssl_config,
      const net::SSLClientSocketContext& context) override;

  // Clears cache used for SSL session resumption.
  void ClearSSLSessionCache() override;

private:
  scoped_refptr<DevToolsURLRequestInterceptor::State> state_;
  net::ClientSocketFactory* base_client_socket_factory_;

};

} // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_DEVTOOLS_CLINET_SOCKET_FACTORY_
