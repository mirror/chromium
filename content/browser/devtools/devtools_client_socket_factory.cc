// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/devtools/devtools_client_socket_factory.h"

#include "content/browser/devtools/devtools_stream_socket.h"
#include "content/browser/devtools/devtools_datagram_client_socket.h"
#include "net/socket/client_socket_handle.h"
#include "net/socket/ssl_client_socket.h"

namespace content {

DevtoolsClientSocketFactory::DevtoolsClientSocketFactory(
    scoped_refptr<DevToolsURLRequestInterceptor::State> state,
    net::ClientSocketFactory* base_client_factory)
    : state_(state)
    , base_client_socket_factory_(base_client_factory) { }

DevtoolsClientSocketFactory::~DevtoolsClientSocketFactory() {}

std::unique_ptr<net::DatagramClientSocket> DevtoolsClientSocketFactory::CreateDatagramClientSocket(
    net::DatagramSocket::BindType bind_type,
    const net::RandIntCallback& rand_int_cb,
    net::NetLog* net_log,
    const net::NetLogSource& source) {
  fprintf(stderr, "CreateDatagramClientSocket was overridden\n");
  return std::make_unique<DevtoolsDatagramClientSocket>(
      base_client_socket_factory_->CreateDatagramClientSocket(
        bind_type, rand_int_cb, net_log, source), state_);
};

std::unique_ptr<net::StreamSocket> DevtoolsClientSocketFactory::CreateTransportClientSocket(
    const net::AddressList& addresses,
    std::unique_ptr<net::SocketPerformanceWatcher> socket_performance_watcher,
    net::NetLog* net_log,
    const net::NetLogSource& source) {
  fprintf(stderr, "CreateTransportClientSocket was overridden\n");
  return std::make_unique<DevtoolsStreamSocket>(
      base_client_socket_factory_->CreateTransportClientSocket(addresses,
        std::move(socket_performance_watcher), net_log, source), state_);
};

std::unique_ptr<net::SSLClientSocket> DevtoolsClientSocketFactory::CreateSSLClientSocket(
    std::unique_ptr<net::ClientSocketHandle> transport_socket,
    const net::HostPortPair& host_and_port,
    const net::SSLConfig& ssl_config,
    const net::SSLClientSocketContext& context) {
  // Should not need to do anything here since the socket should be already
  // from one of the earlier
  // DevtoolsURLClientSocketFactory::CreateTransportClientSocket
  return base_client_socket_factory_->CreateSSLClientSocket(
            std::move(transport_socket), host_and_port, ssl_config,
            context);
};

// Clears cache used for SSL session resumption.
void DevtoolsClientSocketFactory::ClearSSLSessionCache() {
  fprintf(stderr, "ClearSSLSessionCache was overridden\n");
  base_client_socket_factory_->ClearSSLSessionCache();
}

} // namespace content
