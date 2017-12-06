// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_UDP_SOCKET_TEST_UTIL_H_
#define SERVICES_NETWORK_UDP_SOCKET_TEST_UTIL_H_

#include <vector>

#include "base/containers/span.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "net/base/net_errors.h"
#include "services/network/public/interfaces/udp_socket.mojom.h"

namespace network {

namespace test {

// An implementation of mojom::UDPSocketReceiver that records received results.
class UDPSocketReceiverImpl : public mojom::UDPSocketReceiver {
 public:
  struct ReceiveResult {
    ReceiveResult(int net_error_arg,
                  const net::IPEndPoint& src_addr_arg,
                  const std::vector<uint8_t>& data_arg);
    ReceiveResult(const ReceiveResult& other);
    ~ReceiveResult();
    int net_error;
    net::IPEndPoint src_addr;
    std::vector<uint8_t> data;
  };

  UDPSocketReceiverImpl();
  ~UDPSocketReceiverImpl() override;

  const std::vector<ReceiveResult>& results() { return results_; }

  void WaitForReceiveResults(size_t count);

 private:
  void OnReceived(int32_t result,
                  const net::IPEndPoint& src_addr,
                  base::Optional<base::span<const uint8_t>> data) override;
  std::unique_ptr<base::RunLoop> run_loop_;
  std::vector<ReceiveResult> results_;
  size_t expected_receive_count_;

  DISALLOW_COPY_AND_ASSIGN(UDPSocketReceiverImpl);
};

}  // namespace test

}  // namespace network

#endif  // SERVICES_NETWORK_UDP_SOCKET_TEST_UTIL_H_
