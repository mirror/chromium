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
                  const base::Optional<std::vector<uint8_t>>& data) override {
    results_.emplace_back(
        result, std::move(src_addr),
        data ? std::move(data.value()) : std::move(std::vector<uint8_t>()));
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
  mojom::UDPSocketPtr client_socket;
  mojom::UDPSocketRequest request(mojo::MakeRequest(&client_socket));
  net::IPEndPoint remote_addr(GetLocalHostWithAnyPort());
  mojom::UDPSocketReceiverRequest receiver_request;
  int result = UDPSocketImpl::CreateClientSocket(
      std::move(request), &receiver_request, remote_addr);
  ASSERT_EQ(net::OK, result);

  base::RunLoop run_loop;
  client_socket->SetSendBufferSize(1024,
                                   base::BindOnce(
                                       [](base::RunLoop* run_loop, int result) {
                                         EXPECT_EQ(net::OK, result);
                                         run_loop->Quit();
                                       },
                                       base::Unretained(&run_loop)));
  run_loop.Run();

  base::RunLoop run_loop2;
  client_socket->SetReceiveBufferSize(
      2048, base::BindOnce(
                [](base::RunLoop* run_loop, int result) {
                  EXPECT_EQ(net::OK, result);
                  run_loop->Quit();
                },
                base::Unretained(&run_loop2)));
  run_loop2.Run();

  base::RunLoop run_loop3;
  client_socket->NegotiateMaxPendingSendRequests(
      0, base::BindOnce(
             [](base::RunLoop* run_loop, uint32_t result) {
               // TODO(xunjieli): Export default values to tests to avoid
               // hardcoding the value.
               EXPECT_EQ(32u, result);
               run_loop->Quit();
             },
             base::Unretained(&run_loop3)));
  run_loop3.Run();

  base::RunLoop run_loop4;
  client_socket->NegotiateMaxPendingSendRequests(
      16, base::BindOnce(
              [](base::RunLoop* run_loop, uint32_t result) {
                EXPECT_EQ(16u, result);
                run_loop->Quit();
              },
              base::Unretained(&run_loop4)));
  run_loop4.Run();
}

TEST_F(UDPSocketTest, TestReadWrite) {
  // Create a server socket to listen for incoming datagrams.
  UDPSocketReceiverImpl receiver;
  mojo::Binding<mojom::UDPSocketReceiver> receiver_binding(&receiver);
  mojom::UDPSocketReceiverRequest receiver_request;

  net::IPEndPoint server_addr(GetLocalHostWithAnyPort());
  mojom::UDPSocketPtr server_socket;
  mojom::UDPSocketRequest request(mojo::MakeRequest(&server_socket));
  int result = UDPSocketImpl::CreateServerSocket(std::move(request),
                                                 &receiver_request, server_addr,
                                                 /*allow_address_reuse=*/false);
  ASSERT_EQ(net::OK, result);
  receiver_binding.Bind(std::move(receiver_request));

  base::RunLoop run_loop1;
  server_socket->GetLocalAddress(base::BindOnce(
      [](base::RunLoop* run_loop, net::IPEndPoint* out_addr,
         const net::IPEndPoint& result) {
        *out_addr = result;
        run_loop->Quit();
      },
      base::Unretained(&run_loop1), base::Unretained(&server_addr)));
  run_loop1.Run();

  // Create a client socket to send datagrams.
  mojom::UDPSocketPtr client_socket;
  mojom::UDPSocketRequest client_socket_request(
      mojo::MakeRequest(&client_socket));
  mojom::UDPSocketReceiverRequest client_receiver_request;
  result = UDPSocketImpl::CreateClientSocket(
      std::move(client_socket_request), &client_receiver_request, server_addr);
  ASSERT_EQ(net::OK, result);

  const size_t kDatagramCount = 6;
  const size_t kDatagramSize = 255;
  server_socket->ReceiveMore(kDatagramCount);

  for (size_t i = 0; i < kDatagramCount; ++i) {
    base::RunLoop run_loop;
    client_socket->SendTo(
        server_addr, CreateTestMessage(static_cast<uint8_t>(i), kDatagramSize),
        base::BindOnce(
            [](base::RunLoop* run_loop, int result) {
              EXPECT_EQ(static_cast<int>(kDatagramSize), result);
              run_loop->Quit();
            },
            base::Unretained(&run_loop)));
    run_loop.Run();
  }

  net::IPEndPoint client_addr;
  base::RunLoop run_loop2;
  client_socket->GetLocalAddress(base::BindOnce(
      [](base::RunLoop* run_loop, net::IPEndPoint* out_addr,
         const net::IPEndPoint& result) {
        *out_addr = result;
        run_loop->Quit();
      },
      base::Unretained(&run_loop2), base::Unretained(&client_addr)));
  run_loop2.Run();

  receiver.WaitForReceiveResults(kDatagramCount);
  EXPECT_EQ(kDatagramCount, receiver.results().size());

  int i = 0;
  for (const auto& result : receiver.results()) {
    EXPECT_EQ(static_cast<int>(kDatagramSize), result.net_error);
    EXPECT_EQ(result.src_addr, client_addr);
    EXPECT_EQ(result.data,
              CreateTestMessage(static_cast<uint8_t>(i), kDatagramSize));
    i++;
  }
}

}  // namespace network
