// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/nqe/socket_watcher_factory_test_util.h"

#include "base/memory/ptr_util.h"
#include "base/time/time.h"
#include "net/nqe/socket_watcher_test_util.h"

namespace net {

namespace nqe {

namespace internal {

TestSocketWatcherFactory::TestSocketWatcherFactory(
    const SocketWatcherFactory& factory)
    : SocketWatcherFactory(factory) {}

TestSocketWatcherFactory::~TestSocketWatcherFactory() {}

std::unique_ptr<SocketPerformanceWatcher>
TestSocketWatcherFactory::CreateSocketPerformanceWatcher(
    const Protocol protocol,
    const AddressList& address_list) {
  std::unique_ptr<SocketWatcher> watcher = base::MakeUnique<SocketWatcher>(
      protocol, address_list, min_notification_interval_, task_runner_,
      updated_rtt_observation_callback_, tick_clock_);

  return base::MakeUnique<TestSocketWatcher>(*watcher);
}

}  // namespace internal

}  // namespace nqe

}  // namespace net