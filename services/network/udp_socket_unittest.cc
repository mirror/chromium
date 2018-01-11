// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <utility>

#include "services/network/udp_socket.h"

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

class SocketWrapperTestImpl : public UDPSocket::SocketWrapper {
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
  int Write(net::IOBuffer* buf,
            int buf_len,
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

// A Mock UDPSocket that always returns net::ERR_IO_PENDING for SendTo()s.
class HangingUDPSocket : public SocketWrapperTestImpl {
 public:
  HangingUDPSocket() {}

  // SocketWrapperTestImpl implementation.
  int Open(net::AddressFamily address_family) override { return net::OK; }
  int Bind(const net::IPEndPoint& local_addr) override { return net::OK; }
  int SendTo(net::IOBuffer* buf,
             int buf_len,
             const net::IPEndPoint& address,
             const net::CompletionCallback& callback) override {
    if (should_complete_requests_)
      return net::OK;
    pending_send_requests_.push_back(callback);
    return net::ERR_IO_PENDING;
  }
  int GetLocalAddress(net::IPEndPoint* address) const override {
    *address = GetLocalHostWithAnyPort();
    return net::OK;
  }

  // Completes all pending requests.
  void CompleteAllPendingRequests() {
    should_complete_requests_ = true;
    for (auto request : pending_send_requests_) {
      request.Run(net::OK);
    }
    pending_send_requests_.clear();
  }

 private:
  bool should_complete_requests_ = false;
  std::vector<net::CompletionCallback> pending_send_requests_;
};

// A Mock UDPSocket that returns 0 byte read.
class ZeroByteReadUDPSocket : public SocketWrapperTestImpl {
 public:
  ZeroByteReadUDPSocket() {}
  int Open(net::AddressFamily address_family) override { return net::OK; }
  int Bind(const net::IPEndPoint& local_addr) override { return net::OK; }
  int RecvFrom(net::IOBuffer* buf,
               int buf_len,
               net::IPEndPoint* address,
               const net::CompletionCallback& callback) override {
    *address = GetLocalHostWithAnyPort();
    return net::OK;
  }
  int GetLocalAddress(net::IPEndPoint* address) const override {
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
  UDPSocket impl(mojo::MakeRequest(&socket_ptr),
                 std::move(receiver_interface_ptr));
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
}

// Tests that when Send() is used after Bind() is not supported. Send() should
// only be used after Connect().
TEST_F(UDPSocketTest, TestSendWithBind) {
  mojom::UDPSocketReceiverPtr receiver_interface_ptr;
  mojom::UDPSocketPtr socket_ptr;
  UDPSocket impl(mojo::MakeRequest(&socket_ptr),
                 std::move(receiver_interface_ptr));

  net::IPEndPoint server_addr(GetLocalHostWithAnyPort());

  // Bind() the socket.
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::OpenSync(
                         &socket_ptr, server_addr.GetFamily()));
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::BindSync(
                         &socket_ptr, server_addr, &server_addr));

  // Connect() has not been used, so Send() is not supported.
  std::vector<uint8_t> test_msg{1};
  int result = test::UDPSocketTestHelper::SendSync(&socket_ptr, test_msg);
  EXPECT_TRUE(net::ERR_FAILED == result ||
              net::ERR_SOCKET_NOT_CONNECTED == result);
}

// Tests that calling sequences of Open(), Bind()/Connect() and setters are
// important.
TEST_F(UDPSocketTest, TestUnexpectedSequences) {
  mojom::UDPSocketReceiverPtr receiver_interface_ptr;
  mojom::UDPSocketPtr socket_ptr;
  UDPSocket impl(mojo::MakeRequest(&socket_ptr),
                 std::move(receiver_interface_ptr));
  net::IPEndPoint server_addr;
  net::IPEndPoint any_port(GetLocalHostWithAnyPort());

  // Before Open(), calling Connect() or Bind() will result in an error.
  net::IPEndPoint local_addr;
  ASSERT_EQ(net::ERR_UNEXPECTED, test::UDPSocketTestHelper::BindSync(
                                     &socket_ptr, any_port, &local_addr));
  ASSERT_EQ(net::ERR_UNEXPECTED, test::UDPSocketTestHelper::ConnectSync(
                                     &socket_ptr, any_port, &local_addr));

  // Calling any Setters that depend on Open() should fail.
  EXPECT_EQ(
      net::ERR_UNEXPECTED,
      test::UDPSocketTestHelper::SetSendBufferSizeSync(&socket_ptr, 1024));
  EXPECT_EQ(
      net::ERR_UNEXPECTED,
      test::UDPSocketTestHelper::SetReceiveBufferSizeSync(&socket_ptr, 2048));

  // Now call Open().
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::OpenSync(&socket_ptr,
                                                         any_port.GetFamily()));
  EXPECT_NE(local_addr.ToString(), any_port.ToString());

  // It is illegal call Open() twice.
  ASSERT_EQ(net::ERR_UNEXPECTED, test::UDPSocketTestHelper::OpenSync(
                                     &socket_ptr, any_port.GetFamily()));

  // Before Connect()/Bind(), any Setters that depend on Connect()/Bind() should
  // fail.
  EXPECT_EQ(
      net::ERR_UNEXPECTED,
      test::UDPSocketTestHelper::SetSendBufferSizeSync(&socket_ptr, 1024));
  EXPECT_EQ(
      net::ERR_UNEXPECTED,
      test::UDPSocketTestHelper::SetReceiveBufferSizeSync(&socket_ptr, 2048));

  // Now Bind() the socket.
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::BindSync(&socket_ptr, any_port,
                                                         &local_addr));

  // Calling Connect() after Bind() should fail because they can't be both used.
  ASSERT_EQ(net::ERR_SOCKET_IS_CONNECTED,
            test::UDPSocketTestHelper::ConnectSync(&socket_ptr, any_port,
                                                   &local_addr));

  // Now these Setters should work.
  EXPECT_EQ(net::OK, test::UDPSocketTestHelper::SetSendBufferSizeSync(
                         &socket_ptr, 1024));
  EXPECT_EQ(net::OK, test::UDPSocketTestHelper::SetReceiveBufferSizeSync(
                         &socket_ptr, 2048));
}

// Test that exercises the queuing of send requests and makes sure
// ERR_INSUFFICIENT_RESOURCES is returned appropriately.
TEST_F(UDPSocketTest, TestInsufficientResources) {
  mojom::UDPSocketReceiverPtr receiver_interface_ptr;
  mojom::UDPSocketPtr socket_ptr;
  UDPSocket impl(mojo::MakeRequest(&socket_ptr),
                 std::move(receiver_interface_ptr));

  HangingUDPSocket* socket_raw_ptr = new HangingUDPSocket();
  impl.wrapped_socket_ = base::WrapUnique(socket_raw_ptr);

  const size_t kQueueSize = 32;

  net::IPEndPoint server_addr(GetLocalHostWithAnyPort());
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::OpenSync(
                         &socket_ptr, server_addr.GetFamily()));
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::BindSync(
                         &socket_ptr, server_addr, &server_addr));
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
                             EXPECT_EQ(net::OK, result);
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
  // Complete all pending requests. Queued SendTo() should hear back.
  socket_raw_ptr->CompleteAllPendingRequests();
  for (const auto& loop : run_loops) {
    loop->Run();
  }
}

TEST_F(UDPSocketTest, TestReadSend) {
  // Create a server socket to listen for incoming datagrams.
  test::UDPSocketReceiverImpl receiver;
  mojo::Binding<mojom::UDPSocketReceiver> receiver_binding(&receiver);
  mojom::UDPSocketReceiverPtr receiver_interface_ptr;
  receiver_binding.Bind(mojo::MakeRequest(&receiver_interface_ptr));

  mojom::UDPSocketPtr server_socket;
  UDPSocket impl(mojo::MakeRequest(&server_socket),
                 std::move(receiver_interface_ptr));

  net::IPEndPoint server_addr(GetLocalHostWithAnyPort());
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::OpenSync(
                         &server_socket, server_addr.GetFamily()));
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::BindSync(
                         &server_socket, server_addr, &server_addr));

  // Create a client socket to send datagrams.
  mojom::UDPSocketReceiverPtr client_receiver_ptr;
  mojom::UDPSocketPtr client_socket;
  UDPSocket client_impl(mojo::MakeRequest(&client_socket),
                        std::move(client_receiver_ptr));
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
    int result = test::UDPSocketTestHelper::SendSync(&client_socket, test_msg);
    EXPECT_EQ(net::OK, result);
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

TEST_F(UDPSocketTest, TestReadSendTo) {
  // Create a server socket to send data.
  mojom::UDPSocketPtr server_socket;
  mojom::UDPSocketReceiverPtr receiver_ptr;
  UDPSocket impl(mojo::MakeRequest(&server_socket), std::move(receiver_ptr));

  net::IPEndPoint server_addr(GetLocalHostWithAnyPort());
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::OpenSync(
                         &server_socket, server_addr.GetFamily()));
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::BindSync(
                         &server_socket, server_addr, &server_addr));

  // Create a client socket to send datagrams.
  test::UDPSocketReceiverImpl receiver;
  mojo::Binding<mojom::UDPSocketReceiver> receiver_binding(&receiver);
  mojom::UDPSocketReceiverPtr client_receiver_ptr;
  receiver_binding.Bind(mojo::MakeRequest(&client_receiver_ptr));

  mojom::UDPSocketPtr client_socket;
  UDPSocket client_impl(mojo::MakeRequest(&client_socket),
                        std::move(client_receiver_ptr));
  net::IPEndPoint client_addr(GetLocalHostWithAnyPort());
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::OpenSync(
                         &client_socket, client_addr.GetFamily()));
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::ConnectSync(
                         &client_socket, server_addr, &client_addr));

  const size_t kDatagramCount = 6;
  const size_t kDatagramSize = 255;
  client_socket->ReceiveMore(kDatagramCount);

  for (size_t i = 0; i < kDatagramCount; ++i) {
    std::vector<uint8_t> test_msg(
        CreateTestMessage(static_cast<uint8_t>(i), kDatagramSize));
    int result = test::UDPSocketTestHelper::SendToSync(&server_socket,
                                                       client_addr, test_msg);
    EXPECT_EQ(net::OK, result);
  }

  receiver.WaitForReceivedResults(kDatagramCount);
  EXPECT_EQ(kDatagramCount, receiver.results().size());

  int i = 0;
  for (const auto& result : receiver.results()) {
    EXPECT_EQ(net::OK, result.net_error);
    EXPECT_EQ(result.src_addr, server_addr);
    EXPECT_EQ(CreateTestMessage(static_cast<uint8_t>(i), kDatagramSize),
              result.data.value());
    i++;
  }
}

// Make sure the underlying socket implementations(i.e. net::UDPSocketPosix and
// net::UDPSocketWin) handle invalid net::IPEndPoint gracefully.
TEST_F(UDPSocketTest, TestSendToInvalidAddress) {
  mojom::UDPSocketReceiverPtr receiver_interface_ptr;
  mojom::UDPSocketPtr server_socket;
  UDPSocket impl(mojo::MakeRequest(&server_socket),
                 std::move(receiver_interface_ptr));

  net::IPEndPoint server_addr(GetLocalHostWithAnyPort());
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::OpenSync(
                         &server_socket, server_addr.GetFamily()));
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::BindSync(
                         &server_socket, server_addr, &server_addr));

  std::vector<uint8_t> test_msg{1};
  std::vector<uint8_t> invalid_ip_addr{127, 0, 0, 0, 1};
  net::IPAddress ip_address(invalid_ip_addr.data(), invalid_ip_addr.size());
  EXPECT_FALSE(ip_address.IsValid());
  net::IPEndPoint invalid_addr(ip_address, 53);
  server_socket->SendTo(invalid_addr, test_msg,
                        base::BindOnce([](int result) {}));
  // Make sure that the pipe is broken upon processing |invalid_addr|.
  base::RunLoop run_loop;
  server_socket.set_connection_error_handler(
      base::BindOnce([](base::RunLoop* run_loop) { run_loop->Quit(); },
                     base::Unretained(&run_loop)));
  run_loop.Run();
}

// Tests that it is legal for UDPSocketReceiver::OnReceive() to be called with
// 0 byte payload.
TEST_F(UDPSocketTest, TestReadZeroByte) {
  test::UDPSocketReceiverImpl receiver;
  mojo::Binding<mojom::UDPSocketReceiver> receiver_binding(&receiver);
  mojom::UDPSocketReceiverPtr receiver_interface_ptr;
  receiver_binding.Bind(mojo::MakeRequest(&receiver_interface_ptr));

  mojom::UDPSocketPtr socket_ptr;
  UDPSocket impl(mojo::MakeRequest(&socket_ptr),
                 std::move(receiver_interface_ptr));

  impl.wrapped_socket_ = std::make_unique<ZeroByteReadUDPSocket>();

  net::IPEndPoint server_addr(GetLocalHostWithAnyPort());
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::OpenSync(
                         &socket_ptr, server_addr.GetFamily()));
  ASSERT_EQ(net::OK, test::UDPSocketTestHelper::BindSync(
                         &socket_ptr, server_addr, &server_addr));

  socket_ptr->ReceiveMore(1);

  receiver.WaitForReceivedResults(1);
  ASSERT_EQ(1u, receiver.results().size());

  auto result = receiver.results()[0];
  EXPECT_EQ(net::OK, result.net_error);
  EXPECT_TRUE(result.data);
  EXPECT_EQ(std::vector<uint8_t>(), result.data.value());
}

}  // namespace network
