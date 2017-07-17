// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_NQE_SOCKET_WATCHER_FACTORY_TEST_UTIL_H_
#define NET_NQE_SOCKET_WATCHER_FACTORY_TEST_UTIL_H_

#include <memory>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "net/base/net_export.h"
#include "net/nqe/socket_watcher_factory.h"

namespace net {

namespace nqe {

namespace internal {

// SocketWatcherFactory implements SocketPerformanceWatcherFactory.
// SocketWatcherFactory is thread safe.
class NET_EXPORT_PRIVATE TestSocketWatcherFactory
    : public SocketWatcherFactory {
 public:
  TestSocketWatcherFactory(const SocketWatcherFactory& factory);

  ~TestSocketWatcherFactory() override;

  std::unique_ptr<SocketPerformanceWatcher> CreateSocketPerformanceWatcher(
      const Protocol protocol,
      const AddressList& address_list) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(TestSocketWatcherFactory);
};

}  // namespace internal

}  // namespace nqe

}  // namespace net

#endif  // NET_NQE_SOCKET_WATCHER_FACTORY_TEST_UTIL_H_
