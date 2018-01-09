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

net::IPEndPoint GetLocalHostWithAnyPort() {
  return net::IPEndPoint(net::IPAddress(127, 0, 0, 1), 0);
}

}  // namespace

// Tests that when client end of the pipe is closed, the factory implementation
// cleans up the server side of the pipe.
TEST_F(UDPSocketFactoryTest, ConnectionError) {
  mojom::UDPSocketFactoryPtr factory_ptr;
  network::UDPSocketFactory factory;
  factory.AddRequest(mojo::MakeRequest(&factory_ptr));

  mojom::UDPSocketReceiverPtr receiver_interface_ptr;
  mojom::UDPSocketOptionsPtr socket_options;
  mojom::UDPSocketPtr socket_ptr;
  net::IPEndPoint server_addr(GetLocalHostWithAnyPort());

  int result = net::ERR_FAILED;
  base::RunLoop run_loop;
  factory_ptr->OpenAndBind(
      mojo::MakeRequest(&socket_ptr), std::move(socket_options),
      std::move(receiver_interface_ptr), server_addr,
      base::BindOnce(
          [](base::RunLoop* run_loop, int* result_out,
             net::IPEndPoint* local_addr_out, int result,
             const base::Optional<net::IPEndPoint>& local_addr) {
            *result_out = result;
            if (local_addr) {
              *local_addr_out = local_addr.value();
            }
            run_loop->Quit();
          },
          base::Unretained(&run_loop), base::Unretained(&result),
          base::Unretained(&server_addr)));
  run_loop.Run();
  ASSERT_EQ(net::OK, result);

  // Close client side of the pipe.
  socket_ptr.reset();

  base::RunLoop().RunUntilIdle();
  // Make sure the socket is cleaned up.
  EXPECT_EQ(0u, factory.udp_sockets_.size());
}

}  // namespace network
