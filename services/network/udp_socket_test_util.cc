// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/udp_socket_test_util.h"

#include <utility>

#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace test {

UDPSocketReceiverImpl::ReceivedResult::ReceivedResult(
    int net_error_arg,
    const net::IPEndPoint& src_addr_arg,
    base::Optional<std::vector<uint8_t>> data_arg)
    : net_error(net_error_arg), src_addr(src_addr_arg), data(data_arg) {}

UDPSocketReceiverImpl::ReceivedResult::ReceivedResult(
    const ReceivedResult& other) = default;

UDPSocketReceiverImpl::ReceivedResult::~ReceivedResult() {}

UDPSocketReceiverImpl::UDPSocketReceiverImpl()
    : run_loop_(std::make_unique<base::RunLoop>()),
      expected_receive_count_(0) {}

UDPSocketReceiverImpl::~UDPSocketReceiverImpl() {}

void UDPSocketReceiverImpl::WaitForReceivedResults(size_t count) {
  DCHECK_LE(results_.size(), count);
  DCHECK_EQ(0u, expected_receive_count_);

  if (results_.size() == count)
    return;

  expected_receive_count_ = count;
  run_loop_->Run();
  run_loop_ = std::make_unique<base::RunLoop>();
}

void UDPSocketReceiverImpl::OnReceived(
    int32_t result,
    const net::IPEndPoint& src_addr,
    base::Optional<base::span<const uint8_t>> data) {
  DCHECK_GE(0, result);
  if (result == 0)
    DCHECK(data);
  if (result < 0)
    DCHECK(!data);

  results_.emplace_back(result, std::move(src_addr),
                        data ? base::make_optional(std::vector<uint8_t>(
                                   data.value().begin(), data.value().end()))
                             : base::nullopt);
  if (results_.size() == expected_receive_count_) {
    expected_receive_count_ = 0;
    run_loop_->Quit();
  }
}

int UDPSocketTestHelper::OpenSync(mojom::UDPSocketPtr* socket,
                                  net::AddressFamily address_family) {
  base::RunLoop run_loop;
  int result = net::ERR_FAILED;
  socket->get()->Open(
      address_family,
      base::BindOnce(
          [](base::RunLoop* run_loop, int* result_out, int result) {
            *result_out = result;
            run_loop->Quit();
          },
          base::Unretained(&run_loop), base::Unretained(&result)));
  run_loop.Run();
  return result;
}

int UDPSocketTestHelper::ConnectSync(mojom::UDPSocketPtr* socket,
                                     const net::IPEndPoint& remote_addr,
                                     net::IPEndPoint* local_addr_out) {
  base::RunLoop run_loop;
  int result = net::ERR_FAILED;
  socket->get()->Connect(
      remote_addr, base::BindOnce(
                       [](base::RunLoop* run_loop, int* result_out,
                          net::IPEndPoint* local_addr_out, int result,
                          const net::IPEndPoint& local_addr) {
                         *result_out = result;
                         *local_addr_out = local_addr;
                         run_loop->Quit();
                       },
                       base::Unretained(&run_loop), base::Unretained(&result),
                       base::Unretained(local_addr_out)));
  run_loop.Run();
  return result;
}

int UDPSocketTestHelper::BindSync(mojom::UDPSocketPtr* socket,
                                  const net::IPEndPoint& local_addr,
                                  net::IPEndPoint* local_addr_out) {
  base::RunLoop run_loop;
  int result = net::ERR_FAILED;
  net::IPEndPoint real_local_addr;
  socket->get()->Bind(
      local_addr, base::BindOnce(
                      [](base::RunLoop* run_loop, int* result_out,
                         net::IPEndPoint* local_addr_out, int result,
                         const net::IPEndPoint& local_addr) {
                        *result_out = result;
                        *local_addr_out = local_addr;
                        run_loop->Quit();
                      },
                      base::Unretained(&run_loop), base::Unretained(&result),
                      base::Unretained(local_addr_out)));
  run_loop.Run();
  return result;
}

int UDPSocketTestHelper::SendToSync(mojom::UDPSocketPtr* socket,
                                    const net::IPEndPoint& remote_addr,
                                    const std::vector<uint8_t>& input) {
  base::RunLoop run_loop;
  int result = net::ERR_FAILED;
  socket->get()->SendTo(
      remote_addr, input,
      base::BindOnce(
          [](base::RunLoop* run_loop, int* result_out, int result) {
            *result_out = result;
            run_loop->Quit();
          },
          base::Unretained(&run_loop), base::Unretained(&result)));
  run_loop.Run();
  return result;
}

int UDPSocketTestHelper::SetSendBufferSizeSync(mojom::UDPSocketPtr* socket,
                                               size_t size) {
  base::RunLoop run_loop;
  int result = net::ERR_FAILED;
  socket->get()->SetSendBufferSize(
      size, base::BindOnce(
                [](base::RunLoop* run_loop, int* result_out, int result) {
                  *result_out = result;
                  run_loop->Quit();
                },
                base::Unretained(&run_loop), base::Unretained(&result)));
  run_loop.Run();
  return result;
}

int UDPSocketTestHelper::SetReceiveBufferSizeSync(mojom::UDPSocketPtr* socket,
                                                  size_t size) {
  base::RunLoop run_loop;
  int result = net::ERR_FAILED;
  socket->get()->SetReceiveBufferSize(
      size, base::BindOnce(
                [](base::RunLoop* run_loop, int* result_out, int result) {
                  *result_out = result;
                  run_loop->Quit();
                },
                base::Unretained(&run_loop), base::Unretained(&result)));
  run_loop.Run();
  return result;
}

uint32_t UDPSocketTestHelper::NegotiateMaxPendingSendRequestsSync(
    mojom::UDPSocketPtr* socket,
    size_t size) {
  base::RunLoop run_loop;
  uint32_t result_out;
  socket->get()->NegotiateMaxPendingSendRequests(
      size,
      base::BindOnce(
          [](base::RunLoop* run_loop, uint32_t* result_out, uint32_t result) {
            *result_out = result;
            run_loop->Quit();
          },
          base::Unretained(&run_loop), base::Unretained(&result_out)));
  run_loop.Run();
  return result_out;
}

}  // namespace test

}  // namespace network
