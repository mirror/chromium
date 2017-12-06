// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/udp_socket_test_util.h"

#include "base/run_loop.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace network {

namespace test {

UDPSocketReceiverImpl::ReceiveResult::ReceiveResult(
    int net_error_arg,
    const net::IPEndPoint& src_addr_arg,
    const std::vector<uint8_t>& data_arg)
    : net_error(net_error_arg), src_addr(src_addr_arg), data(data_arg) {}

UDPSocketReceiverImpl::ReceiveResult::ReceiveResult(const ReceiveResult& other)
    : net_error(other.net_error), src_addr(other.src_addr), data(other.data) {}

UDPSocketReceiverImpl::ReceiveResult::~ReceiveResult() {}

UDPSocketReceiverImpl::UDPSocketReceiverImpl()
    : run_loop_(std::make_unique<base::RunLoop>()),
      expected_receive_count_(0) {}

UDPSocketReceiverImpl::~UDPSocketReceiverImpl() {}

void UDPSocketReceiverImpl::WaitForReceiveResults(size_t count) {
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
  if (result > 0) {
    ASSERT_NE(base::nullopt, data);
    EXPECT_EQ(static_cast<size_t>(result), data.value().size());
  }
  results_.emplace_back(
      result, std::move(src_addr),
      data ? std::vector<uint8_t>(data.value().begin(), data.value().end())
           : std::vector<uint8_t>());
  if (results_.size() == expected_receive_count_) {
    expected_receive_count_ = 0;
    run_loop_->Quit();
  }
}

}  // namespace test

}  // namespace network
