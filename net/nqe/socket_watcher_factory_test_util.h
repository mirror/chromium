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
#include "net/nqe/socket_watcher_factory.h"

namespace base {
class TickClock;
class TimeDelta;
}  // namespace base

namespace net {

namespace {
typedef base::Callback<void(SocketPerformanceWatcherFactory::Protocol protocol,
                            const base::TimeDelta& rtt)>
    OnUpdatedRTTAvailableCallback;
}

namespace nqe {

namespace internal {

// SocketWatcherFactory implements SocketPerformanceWatcherFactory.
// SocketWatcherFactory is thread safe.
class TestSocketWatcherFactory : public SocketWatcherFactory {
 public:
  // Creates a SocketWatcherFactory.  All socket watchers created by
  // SocketWatcherFactory call |updated_rtt_observation_callback| on
  // |task_runner| every time a new RTT observation is available.
  // |min_notification_interval| is the minimum interval betweeen consecutive
  // notifications to the socket watchers created by this factory. |tick_clock|
  // is guaranteed to be non-null.
  TestSocketWatcherFactory(
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      base::TimeDelta min_notification_interval,
      bool allow_private_sockets_for_testing,
      OnUpdatedRTTAvailableCallback updated_rtt_observation_callback,
      base::TickClock* tick_clock);

  ~TestSocketWatcherFactory() override;

  // SocketPerformanceWatcherFactory implementation:
  std::unique_ptr<SocketPerformanceWatcher> CreateSocketPerformanceWatcher(
      const Protocol protocol,
      const AddressList& address_list) override;

  bool AllowPrivateSockets() const override;

 private:
  bool allow_private_sockets_for_testing_;

  DISALLOW_COPY_AND_ASSIGN(TestSocketWatcherFactory);
};

}  // namespace internal

}  // namespace nqe

}  // namespace net

#endif  // NET_NQE_SOCKET_WATCHER_FACTORY_TEST_UTIL_H_
