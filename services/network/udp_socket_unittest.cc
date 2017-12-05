// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <utility>

#include "services/network/udp_socket_impl.h"

#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "net/base/net_errors.h"
#include "services/network/public/interfaces/udp_socket.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace {

// A Mock UDPClientSocket that always returns net::ERR_IO_PENDING for Write().
class HangingUDPClientSocket : public net::UDPClientSocket {
 public:
  HangingUDPClientSocket()
      : UDPClientSocket(net::DatagramSocket::DEFAULT_BIND,
                        net::RandIntCallback(),
                        nullptr,
                        net::NetLogSource()) {}
  int Write(net::IOBuffer* buf,
            int buf_len,
            const net::CompletionCallback& callback) override {
    return net::ERR_IO_PENDING;
  }
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

struct ReceiveResult {
  ReceiveResult(int net_error_arg,
                const net::IPEndPoint& src_addr_arg,
                std::vector<uint8_t> data_arg)
      : net_error(net_error_arg),
        src_addr(src_addr_arg),
        data(std::move(data_arg)) {}
  int net_error;
  net::IPEndPoint src_addr;
  std::vector<uint8_t> data;
};

class UDPSocketReceiverImpl : public mojom::UDPSocketReceiver {
 public:
  UDPSocketReceiverImpl()
      : run_loop_(std::make_unique<base::RunLoop>()),
        expected_receive_count_(0) {}

  ~UDPSocketReceiverImpl() override {}

  const std::vector<ReceiveResult>& results() { return results_; }

  void WaitForReceiveResults(size_t count) {
    if (results_.size() == count)
      return;

    expected_receive_count_ = count;
    run_loop_->Run();
    run_loop_ = std::make_unique<base::RunLoop>();
  }

 private:
  void OnReceived(int32_t result,
                  const net::IPEndPoint& src_addr,
                  base::Optional<base::span<const uint8_t>> data) override {
    std::vector<uint8_t> data_copy;
    if (result > 0) {
      ASSERT_NE(base::nullopt, data);
      EXPECT_EQ(static_cast<size_t>(result), data.value().size());
      std::vector<uint8_t> value(result);
      memcpy(&value[0], data.value().data(), result);
      std::swap(data_copy, value);
    }
    results_.emplace_back(
        result, std::move(src_addr),
        data ? std::move(data_copy) : std::move(std::vector<uint8_t>()));
    if (results_.size() == expected_receive_count_) {
      expected_receive_count_ = 0;
      run_loop_->Quit();
    }
  }

  std::unique_ptr<base::RunLoop> run_loop_;
  std::vector<ReceiveResult> results_;
  size_t expected_receive_count_;

  DISALLOW_COPY_AND_ASSIGN(UDPSocketReceiverImpl);
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
  net::IPEndPoint any_port(GetLocalHostWithAnyPort());
  net::IPEndPoint server_addr;
  mojom::UDPSocketPtr server_socket;
  mojom::UDPSocketRequest request(mojo::MakeRequest(&server_socket));
  int result = UDPSocketImpl::CreateServerSocket(
      std::move(request), std::move(receiver_interface_ptr), any_port,
      &server_addr, false /*allow_address_reuse*/);
  ASSERT_EQ(net::OK, result);

  {
    base::RunLoop run_loop;
    server_socket->SetSendBufferSize(
        1024, base::BindOnce(
                  [](base::RunLoop* run_loop, int result) {
                    EXPECT_EQ(net::OK, result);
                    run_loop->Quit();
                  },
                  base::Unretained(&run_loop)));
    run_loop.Run();
  }

  {
    base::RunLoop run_loop;
    server_socket->SetReceiveBufferSize(
        2048, base::BindOnce(
                  [](base::RunLoop* run_loop, int result) {
                    EXPECT_EQ(net::OK, result);
                    run_loop->Quit();
                  },
                  base::Unretained(&run_loop)));
    run_loop.Run();
  }

  {
    base::RunLoop run_loop;
    server_socket->NegotiateMaxPendingSendRequests(
        0, base::BindOnce(
               [](base::RunLoop* run_loop, uint32_t result) {
                 // TODO(xunjieli): Export default values to tests to avoid
                 // hardcoding the value.
                 EXPECT_EQ(32u, result);
                 run_loop->Quit();
               },
               base::Unretained(&run_loop)));
    run_loop.Run();
  }

  {
    base::RunLoop run_loop;
    server_socket->NegotiateMaxPendingSendRequests(
        16, base::BindOnce(
                [](base::RunLoop* run_loop, uint32_t result) {
                  EXPECT_EQ(16u, result);
                  run_loop->Quit();
                },
                base::Unretained(&run_loop)));
    run_loop.Run();
  }
}

// Test that exercises the queuing of send requests and makes sure
// ERR_INSUFFICIENT_RESOURCES is returned appropriately.
TEST_F(UDPSocketTest, TestInsufficientResources) {
  mojom::UDPSocketPtr client_socket;
  mojom::UDPSocketRequest client_socket_request(
      mojo::MakeRequest(&client_socket));
  mojom::UDPSocketReceiverPtr client_receiver_ptr;
  auto impl = base::WrapUnique(new UDPSocketImpl(
      true /*is_client_socket*/, std::move(client_receiver_ptr)));
  UDPSocketImpl* impl_raw_ptr = impl.get();
  auto binding = mojo::MakeStrongBinding(std::move(impl),
                                         std::move(client_socket_request));
  impl_raw_ptr->client_socket_ = std::make_unique<HangingUDPClientSocket>();

  const size_t kQueueSize = 10;
  {
    base::RunLoop run_loop;
    client_socket->NegotiateMaxPendingSendRequests(
        kQueueSize, base::BindOnce(
                        [](base::RunLoop* run_loop, uint32_t result) {
                          EXPECT_EQ(10u, result);
                          run_loop->Quit();
                        },
                        base::Unretained(&run_loop)));
    run_loop.Run();
  }

  const size_t kDatagramSize = 255;
  // Send |kQueueSize| + 1 datagrams in a row, so the queue is filled up.
  std::vector<std::unique_ptr<base::RunLoop>> run_loops;
  for (size_t i = 0; i < kQueueSize + 1; ++i) {
    std::vector<uint8_t> test_msg(
        CreateTestMessage(static_cast<uint8_t>(i), kDatagramSize));
    run_loops.push_back(std::make_unique<base::RunLoop>());
    client_socket->SendTo(GetLocalHostWithAnyPort(), test_msg,
                          base::BindOnce(
                              [](base::RunLoop* run_loop, int result) {
                                EXPECT_EQ(net::ERR_INSUFFICIENT_RESOURCES,
                                          result);
                                run_loop->Quit();
                              },
                              base::Unretained(run_loops[i].get())));
  }

  // SendTo() beyond the queue size should fail.
  {
    base::RunLoop run_loop;
    std::vector<uint8_t> test_msg{1};
    client_socket->SendTo(GetLocalHostWithAnyPort(), test_msg,
                          base::BindOnce(
                              [](base::RunLoop* run_loop, int result) {
                                EXPECT_EQ(net::ERR_INSUFFICIENT_RESOURCES,
                                          result);
                                run_loop->Quit();
                              },
                              base::Unretained(&run_loop)));
    run_loop.Run();
  }

  EXPECT_EQ(kQueueSize, impl_raw_ptr->pending_send_requests_.size());
  // Now half the queue size. Queued SendTo() should hear back.
  {
    base::RunLoop run_loop;
    client_socket->NegotiateMaxPendingSendRequests(
        kQueueSize / 2, base::BindOnce(
                            [](base::RunLoop* run_loop, uint32_t result) {
                              EXPECT_EQ(5u, result);
                              run_loop->Quit();
                            },
                            base::Unretained(&run_loop)));
    run_loop.Run();
  }

  for (size_t i = 0; i < kQueueSize / 5; ++i) {
    run_loops[run_loops.size() - 1 - i]->Run();
  }

  // Make sure the queue is halved.
  EXPECT_EQ(kQueueSize / 2, impl_raw_ptr->pending_send_requests_.size());
}

TEST_F(UDPSocketTest, TestReadWrite) {
  // Create a server socket to listen for incoming datagrams.
  UDPSocketReceiverImpl receiver;
  mojo::Binding<mojom::UDPSocketReceiver> receiver_binding(&receiver);
  mojom::UDPSocketReceiverPtr receiver_interface_ptr;
  receiver_binding.Bind(mojo::MakeRequest(&receiver_interface_ptr));

  net::IPEndPoint server_addr(GetLocalHostWithAnyPort());
  net::IPEndPoint real_server_addr;
  mojom::UDPSocketPtr server_socket;
  mojom::UDPSocketRequest request(mojo::MakeRequest(&server_socket));
  int result = UDPSocketImpl::CreateServerSocket(
      std::move(request), std::move(receiver_interface_ptr), server_addr,
      &real_server_addr, false /*allow_address_reuse*/);
  ASSERT_EQ(net::OK, result);
  EXPECT_NE(server_addr.ToString(), real_server_addr.ToString());

  // Create a client socket to send datagrams.
  mojom::UDPSocketPtr client_socket;
  mojom::UDPSocketRequest client_socket_request(
      mojo::MakeRequest(&client_socket));
  mojom::UDPSocketReceiverPtr client_receiver_ptr;
  net::IPEndPoint client_addr;
  result = UDPSocketImpl::CreateClientSocket(std::move(client_socket_request),
                                             std::move(client_receiver_ptr),
                                             real_server_addr, &client_addr);
  ASSERT_EQ(net::OK, result);

  const size_t kDatagramCount = 6;
  const size_t kDatagramSize = 255;
  server_socket->ReceiveMore(kDatagramCount);

  for (size_t i = 0; i < kDatagramCount; ++i) {
    std::vector<uint8_t> test_msg(
        CreateTestMessage(static_cast<uint8_t>(i), kDatagramSize));
    base::RunLoop run_loop;
    client_socket->SendTo(server_addr, test_msg,
                          base::BindOnce(
                              [](base::RunLoop* run_loop, int result) {
                                EXPECT_EQ(static_cast<int>(kDatagramSize),
                                          result);
                                run_loop->Quit();
                              },
                              base::Unretained(&run_loop)));
    run_loop.Run();
  }

  receiver.WaitForReceiveResults(kDatagramCount);
  EXPECT_EQ(kDatagramCount, receiver.results().size());

  int i = 0;
  for (const auto& result : receiver.results()) {
    EXPECT_EQ(static_cast<int>(kDatagramSize), result.net_error);
    EXPECT_EQ(result.src_addr, client_addr);
    EXPECT_EQ(CreateTestMessage(static_cast<uint8_t>(i), kDatagramSize),
              result.data);
    i++;
  }
}

}  // namespace network
