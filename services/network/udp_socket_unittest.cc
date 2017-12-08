// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <utility>

#include "services/network/udp_socket_impl.h"

#include "base/logging.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "net/base/net_errors.h"
#include "net/socket/udp_socket.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/interfaces/udp_socket.mojom.h"
#include "services/network/udp_socket_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace {

class SocketWrapperTestImpl : public UDPSocketImpl::SocketWrapper {
 public:
  SocketWrapperTestImpl() {}
  ~SocketWrapperTestImpl() override {}

  int Open(net::AddressFamily address_family) override {
    NOTREACHED();
    return net::ERR_NOT_IMPLEMENTED;
  }
  int Connect(const net::IPEndPoint& remote_addr) override {
    NOTREACHED();
    return net::ERR_NOT_IMPLEMENTED;
  }
  int Bind(const net::IPEndPoint& local_addr) override {
    NOTREACHED();
    return net::ERR_NOT_IMPLEMENTED;
  }
  int SetSendBufferSize(uint32_t size) override {
    NOTREACHED();
    return net::ERR_NOT_IMPLEMENTED;
  }
  int SetReceiveBufferSize(uint32_t size) override {
    NOTREACHED();
    return net::ERR_NOT_IMPLEMENTED;
  }
  int SendTo(net::IOBuffer* buf,
             int buf_len,
             const net::IPEndPoint& dest_addr,
             const net::CompletionCallback& callback) override {
    NOTREACHED();
    return net::ERR_NOT_IMPLEMENTED;
  }

  int RecvFrom(net::IOBuffer* buf,
               int buf_len,
               net::IPEndPoint* address,
               const net::CompletionCallback& callback) override {
    NOTREACHED();
    return net::ERR_NOT_IMPLEMENTED;
  }

  int GetLocalAddress(net::IPEndPoint* address) const override {
    NOTREACHED();
    return net::ERR_NOT_IMPLEMENTED;
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(SocketWrapperTestImpl);
};

net::IPEndPoint GetLocalHostWithAnyPort() {
  return net::IPEndPoint(net::IPAddress(127, 0, 0, 1), 0);
}

std::vector<uint8_t> CreateTestMessage(uint8_t initial, size_t size) {
  std::vector<uint8_t> array(size);
  for (size_t i = 0; i < size; ++i)
    array[i] = static_cast<uint8_t>((i + initial) % 256);
  return array;
}

// A Mock UDPSocket that always returns net::ERR_IO_PENDING for Write().
class HangingUDPSocket : public SocketWrapperTestImpl {
 public:
  HangingUDPSocket() {}
  int SendTo(net::IOBuffer* buf,
             int buf_len,
             const net::IPEndPoint& address,
             const net::CompletionCallback& callback) override {
    return net::ERR_IO_PENDING;
  }
};

// A Mock UDPSocket that returns 0 byte read.
class ZeroByteReadUDPSocket : public SocketWrapperTestImpl {
 public:
  ZeroByteReadUDPSocket() {}
  int RecvFrom(net::IOBuffer* buf,
               int buf_len,
               net::IPEndPoint* address,
               const net::CompletionCallback& callback) override {
    *address = GetLocalHostWithAnyPort();
    return net::OK;
  }
};

class UDPSocketTest : public testing::Test {
 public:
  UDPSocketTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::IO) {}
  ~UDPSocketTest() override {}

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;

  DISALLOW_COPY_AND_ASSIGN(UDPSocketTest);
};

}  // namespace

TEST_F(UDPSocketTest, Settings) {
  mojom::UDPSocketReceiverPtr receiver_interface_ptr;
  mojom::UDPSocketPtr socket_ptr;
  UDPSocketImpl impl(std::move(receiver_interface_ptr));
  mojo::Binding<mojom::UDPSocket> socket_binding(&impl);
  socket_binding.Bind(mojo::MakeRequest(&socket_ptr));

  net::IPEndPoint server_addr;
  net::IPEndPoint any_port(GetLocalHostWithAnyPort());

  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::OpenSync(&socket_ptr,
                                                         any_port.GetFamily()));
  net::IPEndPoint local_addr;
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::BindSync(&socket_ptr, any_port,
                                                         &local_addr));
  EXPECT_NE(local_addr.ToString(), any_port.ToString());
  EXPECT_EQ(net::OK, test::UDPSocketTestHelper::SetSendBufferSizeSync(
                         &socket_ptr, 1024));
  EXPECT_EQ(net::OK, test::UDPSocketTestHelper::SetReceiveBufferSizeSync(
                         &socket_ptr, 2048));
  EXPECT_EQ(32u, test::UDPSocketTestHelper::NegotiateMaxPendingSendRequestsSync(
                     &socket_ptr, 0));
  EXPECT_EQ(16u, test::UDPSocketTestHelper::NegotiateMaxPendingSendRequestsSync(
                     &socket_ptr, 16));
}

// Test that exercises the queuing of send requests and makes sure
// ERR_INSUFFICIENT_RESOURCES is returned appropriately.
TEST_F(UDPSocketTest, TestInsufficientResources) {
  mojom::UDPSocketReceiverPtr receiver_interface_ptr;
  mojom::UDPSocketPtr socket_ptr;
  UDPSocketImpl impl(std::move(receiver_interface_ptr));
  mojo::Binding<mojom::UDPSocket> socket_binding(&impl);
  socket_binding.Bind(mojo::MakeRequest(&socket_ptr));

  impl.socket_ = std::make_unique<HangingUDPSocket>();

  const size_t kQueueSize = 10;
  EXPECT_EQ(10u, test::UDPSocketTestHelper::NegotiateMaxPendingSendRequestsSync(
                     &socket_ptr, kQueueSize));

  const size_t kDatagramSize = 255;
  // Send |kQueueSize| + 1 datagrams in a row, so the queue is filled up.
  std::vector<std::unique_ptr<base::RunLoop>> run_loops;
  for (size_t i = 0; i < kQueueSize + 1; ++i) {
    std::vector<uint8_t> test_msg(
        CreateTestMessage(static_cast<uint8_t>(i), kDatagramSize));
    run_loops.push_back(std::make_unique<base::RunLoop>());
    socket_ptr->SendTo(GetLocalHostWithAnyPort(), test_msg,
                       base::BindOnce(
                           [](base::RunLoop* run_loop, int result) {
                             EXPECT_EQ(net::ERR_INSUFFICIENT_RESOURCES, result);
                             run_loop->Quit();
                           },
                           base::Unretained(run_loops[i].get())));
  }

  // SendTo() beyond the queue size should fail.
  std::vector<uint8_t> test_msg{1};
  EXPECT_EQ(net::ERR_INSUFFICIENT_RESOURCES,
            test::UDPSocketTestHelper::SendToSync(
                &socket_ptr, GetLocalHostWithAnyPort(), test_msg));

  EXPECT_EQ(kQueueSize, impl.pending_send_requests_.size());
  // Now half the queue size. Queued SendTo() should hear back.
  EXPECT_EQ(kQueueSize / 2,
            test::UDPSocketTestHelper::NegotiateMaxPendingSendRequestsSync(
                &socket_ptr, kQueueSize / 2));

  for (size_t i = 0; i < kQueueSize / 5; ++i) {
    run_loops[run_loops.size() - 1 - i]->Run();
  }

  // Make sure the queue is halved.
  EXPECT_EQ(kQueueSize / 2, impl.pending_send_requests_.size());
}

TEST_F(UDPSocketTest, TestReadWrite) {
  // Create a server socket to listen for incoming datagrams.
  test::UDPSocketReceiverImpl receiver;
  mojo::Binding<mojom::UDPSocketReceiver> receiver_binding(&receiver);
  mojom::UDPSocketReceiverPtr receiver_interface_ptr;
  receiver_binding.Bind(mojo::MakeRequest(&receiver_interface_ptr));

  UDPSocketImpl impl(std::move(receiver_interface_ptr));
  mojo::Binding<mojom::UDPSocket> socket_binding(&impl);
  mojom::UDPSocketPtr server_socket;
  socket_binding.Bind(mojo::MakeRequest(&server_socket));

  net::IPEndPoint server_addr(GetLocalHostWithAnyPort());
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::OpenSync(
                         &server_socket, server_addr.GetFamily()));
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::BindSync(
                         &server_socket, server_addr, &server_addr));

  // Create a client socket to send datagrams.
  mojom::UDPSocketReceiverPtr client_receiver_ptr;
  UDPSocketImpl client_impl(std::move(client_receiver_ptr));
  mojo::Binding<mojom::UDPSocket> client_socket_binding(&client_impl);
  mojom::UDPSocketPtr client_socket;
  client_socket_binding.Bind(mojo::MakeRequest(&client_socket));

  net::IPEndPoint client_addr(GetLocalHostWithAnyPort());
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::OpenSync(
                         &client_socket, client_addr.GetFamily()));
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::ConnectSync(
                         &client_socket, server_addr, &client_addr));

  const size_t kDatagramCount = 6;
  const size_t kDatagramSize = 255;
  server_socket->ReceiveMore(kDatagramCount);

  for (size_t i = 0; i < kDatagramCount; ++i) {
    std::vector<uint8_t> test_msg(
        CreateTestMessage(static_cast<uint8_t>(i), kDatagramSize));
    int result = test::UDPSocketTestHelper::SendToSync(&client_socket,
                                                       server_addr, test_msg);
    EXPECT_EQ(static_cast<int>(kDatagramSize), result);
  }

  receiver.WaitForReceivedResults(kDatagramCount);
  EXPECT_EQ(kDatagramCount, receiver.results().size());

  int i = 0;
  for (const auto& result : receiver.results()) {
    EXPECT_EQ(net::OK, result.net_error);
    EXPECT_EQ(result.src_addr, client_addr);
    EXPECT_EQ(CreateTestMessage(static_cast<uint8_t>(i), kDatagramSize),
              result.data.value());
    i++;
  }
}

// Tests that it is legal for UDPSocketReceiver::OnReceive() to be called with
// 0 byte payload.
TEST_F(UDPSocketTest, TestReadZeroByte) {
  test::UDPSocketReceiverImpl receiver;
  mojo::Binding<mojom::UDPSocketReceiver> receiver_binding(&receiver);
  mojom::UDPSocketReceiverPtr receiver_interface_ptr;
  receiver_binding.Bind(mojo::MakeRequest(&receiver_interface_ptr));

  UDPSocketImpl impl(std::move(receiver_interface_ptr));
  mojo::Binding<mojom::UDPSocket> socket_binding(&impl);
  mojom::UDPSocketPtr client_socket;
  socket_binding.Bind(mojo::MakeRequest(&client_socket));

  impl.socket_ = std::make_unique<ZeroByteReadUDPSocket>();

  client_socket->ReceiveMore(1);

  receiver.WaitForReceivedResults(1);
  ASSERT_EQ(1u, receiver.results().size());

  auto result = receiver.results()[0];
  EXPECT_EQ(net::OK, result.net_error);
  EXPECT_TRUE(result.data);
  EXPECT_EQ(std::vector<uint8_t>(), result.data.value());
}

}  // namespace network
