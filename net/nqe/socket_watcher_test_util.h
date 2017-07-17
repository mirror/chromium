// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_NQE_SOCKET_WATCHER_TEST_UTIL_H_
#define NET_NQE_SOCKET_WATCHER_TEST_UTIL_H_

#include <memory>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "net/base/net_export.h"
#include "net/nqe/socket_watcher.h"

namespace net {

namespace nqe {

namespace internal {

// SocketWatcher implements SocketPerformanceWatcher, and is not thread-safe.
class NET_EXPORT_PRIVATE TestSocketWatcher : public SocketWatcher {
 public:
  // Creates a SocketWatcher which can be used to watch a socket that uses
  // |protocol| as the transport layer protocol. The socket watcher will call
  // |updated_rtt_observation_callback| on |task_runner| every time a new RTT
  // observation is available. |min_notification_interval| is the minimum
  // interval betweeen consecutive notifications to this socket watcher.
  // |tick_clock| is guaranteed to be non-null.
  TestSocketWatcher(
      SocketPerformanceWatcherFactory::Protocol protocol,
      const AddressList& address_list,
      base::TimeDelta min_notification_interval,
      scoped_refptr<base::SingleThreadTaskRunner> task_runner,
      OnUpdatedRTTAvailableCallback updated_rtt_observation_callback,
      base::TickClock* tick_clock);

  ~TestSocketWatcher() override;

  void set_allow_private_sockets_for_testing() {
    allow_private_sockets_for_testing_ = true;
  }

 private:
  void SetAllowPrivateSocketsForTesting() override;

  bool AllowPrivateSockets() override;

  bool allow_private_sockets_for_testing_;
  DISALLOW_COPY_AND_ASSIGN(TestSocketWatcher);
};

}  // namespace internal

}  // namespace nqe

}  // namespace net

#endif  // NET_NQE_SOCKET_WATCHER_TEST_UTIL_H_
