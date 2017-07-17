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
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    base::TimeDelta min_notification_interval,
    bool allow_private_sockets_for_testing,
    OnUpdatedRTTAvailableCallback updated_rtt_observation_callback,
    base::TickClock* tick_clock)
    : SocketWatcherFactory(task_runner,
                           min_notification_interval,
                           updated_rtt_observation_callback,
                           tick_clock) {
  allow_private_sockets_for_testing_ = allow_private_sockets_for_testing;
  DCHECK(allow_private_sockets_for_testing_ == true ||
         allow_private_sockets_for_testing_ == false);
}

TestSocketWatcherFactory::~TestSocketWatcherFactory() {}

std::unique_ptr<SocketPerformanceWatcher>
TestSocketWatcherFactory::CreateSocketPerformanceWatcher(
    const Protocol protocol,
    const AddressList& address_list) {
  std::unique_ptr<SocketPerformanceWatcher> watcher =
      SocketWatcherFactory::CreateSocketPerformanceWatcher(protocol,
                                                           address_list);
  if (AllowPrivateSockets())
    watcher->SetAllowPrivateSocketsForTesting();
  return watcher;
}

bool TestSocketWatcherFactory::AllowPrivateSockets() const {
  return true;
}

}  // namespace internal

}  // namespace nqe

}  // namespace net