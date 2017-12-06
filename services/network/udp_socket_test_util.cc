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

}  // namespace test

}  // namespace network
