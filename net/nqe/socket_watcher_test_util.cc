// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/nqe/socket_watcher_test_util.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/time/tick_clock.h"
#include "base/time/time.h"
#include "net/base/address_list.h"
#include "net/base/ip_address.h"

namespace net {

namespace nqe {

namespace internal {

TestSocketWatcher::TestSocketWatcher(
    SocketPerformanceWatcherFactory::Protocol protocol,
    const AddressList& address_list,
    base::TimeDelta min_notification_interval,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    OnUpdatedRTTAvailableCallback updated_rtt_observation_callback,
    base::TickClock* tick_clock)
    : SocketWatcher(protocol,
                    address_list,
                    min_notification_interval,
                    task_runner,
                    updated_rtt_observation_callback,
                    tick_clock) {
  allow_private_sockets_for_testing_ = false;
}

TestSocketWatcher::~TestSocketWatcher() {}

void TestSocketWatcher::SetAllowPrivateSocketsForTesting() {
  allow_private_sockets_for_testing_ = true;
}

bool TestSocketWatcher::AllowPrivateSockets() {
  return allow_private_sockets_for_testing_;
}

}  // namespace internal

}  // namespace nqe

}  // namespace net
