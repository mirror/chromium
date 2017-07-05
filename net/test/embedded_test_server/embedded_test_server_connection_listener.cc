// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/test/embedded_test_server/embedded_test_server_connection_listener.h"
#include "net/socket/stream_socket.h"

namespace net {

namespace test_server {

void EmbeddedTestServerConnectionListener::AcceptedSocket(
    const StreamSocket& socket) {}

bool EmbeddedTestServerConnectionListener::ConnectionEstablished(
    StreamSocket* socket) {
  return false;
}

void EmbeddedTestServerConnectionListener::ReadFromSocket(
    const StreamSocket& socket,
    int rv) {}

}  // namespace test_server
}  // namespace net
