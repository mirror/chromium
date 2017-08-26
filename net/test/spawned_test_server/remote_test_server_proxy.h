// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TEST_SPAWNED_TEST_SERVER_REMOTE_TEST_SERVER_PROXY_H_
#define NET_TEST_SPAWNED_TEST_SERVER_REMOTE_TEST_SERVER_PROXY_H_

#include <stdint.h>
#include <memory>
#include <vector>

#include "net/socket/tcp_server_socket.h"

namespace net {

class IPEndPoint;

// RemoteTestServerProxy proxies TCP connection from localhost to a remote IP
// address.
class RemoteTestServerProxy {
 public:
  explicit RemoteTestServerProxy(const IPEndPoint& remote_address);
  ~RemoteTestServerProxy();

  uint16_t local_port() { return local_port_; }

 private:
  class ConnectionProxy;

  void DoAccept();
  void OnAcceptResult(int result);
  void HandleAcceptResult(int result);
  void OnConnectionDone(ConnectionProxy* connection);

  IPEndPoint remote_address_;

  TCPServerSocket socket_;

  uint16_t local_port_;
  std::vector<std::unique_ptr<ConnectionProxy>> connections_;

  std::unique_ptr<StreamSocket> accepted_socket_;

  DISALLOW_COPY_AND_ASSIGN(RemoteTestServerProxy);
};

}  // namespace net

#endif  // NET_TEST_SPAWNED_TEST_SERVER_REMOTE_TEST_SERVER_PROXY_H_
