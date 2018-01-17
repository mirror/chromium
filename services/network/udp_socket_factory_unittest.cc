// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>

#include "services/network/udp_socket_factory.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "net/base/net_errors.h"
#include "services/network/public/interfaces/udp_socket.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace {
class UDPSocketFactoryTest : public testing::Test {
 public:
  UDPSocketFactoryTest() {}
  ~UDPSocketFactoryTest() override {}

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  DISALLOW_COPY_AND_ASSIGN(UDPSocketFactoryTest);
};

}  // namespace

// Tests that when client end of the pipe is closed, the factory implementation
// cleans up the server side of the pipe.
TEST_F(UDPSocketFactoryTest, ConnectionError) {
  UDPSocketFactory factory;

  mojom::UDPSocketReceiverPtr receiver_interface_ptr;
  mojom::UDPSocketPtr socket_ptr;

  factory.CreateUDPSocket(mojo::MakeRequest(&socket_ptr),
                          std::move(receiver_interface_ptr));
  EXPECT_EQ(1u, factory.udp_sockets_.size());

  // Close client side of the pipe.
  socket_ptr.reset();

  base::RunLoop().RunUntilIdle();
  // Make sure the socket is cleaned up.
  EXPECT_EQ(0u, factory.udp_sockets_.size());
}

}  // namespace network
