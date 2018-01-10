// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TEST_TCP_SOCKET_PROXY_H_
#define NET_TEST_TCP_SOCKET_PROXY_H_

#include <stdint.h>

#include <memory>

#include "base/memory/ref_counted.h"
#include "base/threading/thread_checker.h"

namespace base {
class SingleThreadTaskRunner;
}  // namespace base

namespace net {

class IPEndPoint;

// TcpSocketProxy proxies TCP connection from localhost to a remote IP address.
class TcpSocketProxy {
 public:
  TcpSocketProxy(scoped_refptr<base::SingleThreadTaskRunner> io_task_runner,
                 int local_port);
  explicit TcpSocketProxy(
      scoped_refptr<base::SingleThreadTaskRunner> io_task_runner);

  ~TcpSocketProxy();

  uint16_t local_port() const { return local_port_; }

  // Starts the proxy for the specified |remote_address|. Must be called before
  // any incoming connection on local_port() are initiated.
  void Start(const IPEndPoint& remote_address);

 private:
  class Core;

  scoped_refptr<base::SingleThreadTaskRunner> io_task_runner_;

  // Core implements the proxy functionality. It runs on |io_task_runner_|.
  std::unique_ptr<Core> core_;

  uint16_t local_port_;

  THREAD_CHECKER(thread_checker_);

  DISALLOW_COPY_AND_ASSIGN(TcpSocketProxy);
};

}  // namespace net

#endif  // NET_TEST_TCP_SOCKET_PROXY_H_
