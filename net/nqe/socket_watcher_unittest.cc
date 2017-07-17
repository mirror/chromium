// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/nqe/socket_watcher.h"

#include "base/bind.h"
#include "base/test/simple_test_tick_clock.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "net/base/address_list.h"
#include "net/base/ip_address.h"
#include "net/socket/socket_performance_watcher.h"
#include "net/socket/socket_performance_watcher_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

namespace nqe {

namespace internal {

namespace {

void OnUpdatedRTTAvailable(SocketPerformanceWatcherFactory::Protocol protocol,
                           const base::TimeDelta& rtt) {}

// Verify that the buffer size is never exceeded.
TEST(NetworkQualitySocketWatcherTest, NotificationsThrottled) {
  base::SimpleTestTickClock tick_clock;
  tick_clock.SetNowTicks(base::TimeTicks::Now());

  const std::string kCanonicalName = "canonical.example.com";
  AddressList address_list;
  IPAddressList ip_list;
  IPAddress ip_address;
  ASSERT_TRUE(ip_address.AssignFromIPLiteral("157.0.0.1"));
  ip_list.push_back(ip_address);
  address_list = AddressList::CreateFromIPAddressList(ip_list, kCanonicalName);

  SocketWatcher socket_watcher(SocketPerformanceWatcherFactory::PROTOCOL_QUIC,
                               address_list,
                               base::TimeDelta::FromMilliseconds(2000),
                               base::ThreadTaskRunnerHandle::Get(),
                               base::Bind(OnUpdatedRTTAvailable), &tick_clock);

  EXPECT_TRUE(socket_watcher.ShouldNotifyUpdatedRTT());
  socket_watcher.OnUpdatedRTTAvailable(base::TimeDelta::FromSeconds(10));

  EXPECT_FALSE(socket_watcher.ShouldNotifyUpdatedRTT());

  tick_clock.Advance(base::TimeDelta::FromMilliseconds(1000));
  // Minimum interval between consecutive notifications is 2000 msec.
  EXPECT_FALSE(socket_watcher.ShouldNotifyUpdatedRTT());

  // Advance the clock by 1000 msec more so that the current time is at least
  // 2000 msec more than the last time |socket_watcher| received a notification.
  tick_clock.Advance(base::TimeDelta::FromMilliseconds(1000));
  EXPECT_TRUE(socket_watcher.ShouldNotifyUpdatedRTT());
}

/*
TEST(NetworkQualitySocketWatcherTest, PrivateSocketsNotNotified) {
  base::SimpleTestTickClock tick_clock;
  tick_clock.SetNowTicks(base::TimeTicks::Now());

  const struct {
    std::string ip_address;
    bool allow_private_sockets_for_testing;
    bool expect_should_notify_rtt;
  } tests[] = {
      {"157.0.0.1", false, true},
      {"157.0.0.1", true, true},
      {"127.0.0.1", false, false},
      {"127.0.0.1", true, true},
  };

  for (const auto& test : tests) {
    const std::string kCanonicalName = "canonical.example.com";
    AddressList address_list;
    IPAddressList ip_list;
    IPAddress ip_address;
    ASSERT_TRUE(ip_address.AssignFromIPLiteral(test.ip_address));
    ip_list.push_back(ip_address);
    address_list =
        AddressList::CreateFromIPAddressList(ip_list, kCanonicalName);

    SocketWatcher socket_watcher(
        SocketPerformanceWatcherFactory::PROTOCOL_QUIC, address_list,
        test.allow_private_sockets_for_testing,
        base::TimeDelta::FromMilliseconds(2000),
        base::ThreadTaskRunnerHandle::Get(), base::Bind(OnUpdatedRTTAvailable),
        &tick_clock);

    EXPECT_EQ(test.expect_should_notify_rtt,
              socket_watcher.ShouldNotifyUpdatedRTT());
    socket_watcher.OnUpdatedRTTAvailable(base::TimeDelta::FromSeconds(10));

    EXPECT_FALSE(socket_watcher.ShouldNotifyUpdatedRTT());
  }
}
*/

}  // namespace

}  // namespace internal

}  // namespace nqe

}  // namespace net