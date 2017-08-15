// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/nqe/socket_watcher.h"

#include "base/bind.h"
#include "base/test/scoped_task_environment.h"
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

class NetworkQualitySocketWatcherTest : public testing::Test {
 protected:
  NetworkQualitySocketWatcherTest() { ResetExpectedCallbackParams(); }
  ~NetworkQualitySocketWatcherTest() override { ResetExpectedCallbackParams(); }

  static void OnUpdatedRTTAvailableStoreParams(
      SocketPerformanceWatcherFactory::Protocol protocol,
      const base::TimeDelta& rtt,
      const base::Optional<SubnetID>& subnet_id) {
    // Need to verify before another callback is executed, or explicitly call
    // |ResetCallbackParams()|.
    ASSERT_FALSE(callback_executed_);
    callback_rtt_ = rtt;
    callback_subnet_id_ = subnet_id;
    callback_executed_ = true;
  }

  static void OnUpdatedRTTAvailable(
      SocketPerformanceWatcherFactory::Protocol protocol,
      const base::TimeDelta& rtt,
      const base::Optional<SubnetID>& subnet_id) {
    // Need to verify before another callback is executed, or explicitly call
    // |ResetCallbackParams()|.
    ASSERT_FALSE(callback_executed_);
    callback_executed_ = true;
  }

  static void VerifyCallbackParams(const base::TimeDelta& rtt,
                                   const base::Optional<SubnetID>& subnet_id) {
    ASSERT_TRUE(callback_executed_);
    EXPECT_EQ(rtt, callback_rtt_);
    if (subnet_id)
      EXPECT_EQ(subnet_id.value_or(0), callback_subnet_id_.value_or(1));
    else
      EXPECT_FALSE(callback_subnet_id_.has_value());
    ResetExpectedCallbackParams();
  }

  static void ResetExpectedCallbackParams() {
    callback_rtt_ = base::TimeDelta::FromMilliseconds(0);
    callback_subnet_id_ = base::nullopt;
    callback_executed_ = false;
  }

 private:
  static base::TimeDelta callback_rtt_;
  static base::Optional<SubnetID> callback_subnet_id_;
  static bool callback_executed_;
};

base::TimeDelta NetworkQualitySocketWatcherTest::callback_rtt_ =
    base::TimeDelta::FromMilliseconds(0);

base::Optional<SubnetID> NetworkQualitySocketWatcherTest::callback_subnet_id_ =
    base::nullopt;

bool NetworkQualitySocketWatcherTest::callback_executed_ = false;

// Verify that the buffer size is never exceeded.
TEST_F(NetworkQualitySocketWatcherTest, NotificationsThrottled) {
  base::SimpleTestTickClock tick_clock;
  tick_clock.SetNowTicks(base::TimeTicks::Now());

  // Use a public IP address so that the socket watcher runs the RTT callback.
  IPAddressList ip_list;
  IPAddress ip_address;
  ASSERT_TRUE(ip_address.AssignFromIPLiteral("157.0.0.1"));
  ip_list.push_back(ip_address);
  AddressList address_list =
      AddressList::CreateFromIPAddressList(ip_list, "canonical.example.com");

  SocketWatcher socket_watcher(SocketPerformanceWatcherFactory::PROTOCOL_QUIC,
                               address_list,

                               base::TimeDelta::FromMilliseconds(2000), false,
                               base::ThreadTaskRunnerHandle::Get(),
                               base::Bind(OnUpdatedRTTAvailable), &tick_clock);

  EXPECT_TRUE(socket_watcher.ShouldNotifyUpdatedRTT());
  socket_watcher.OnUpdatedRTTAvailable(base::TimeDelta::FromSeconds(10));
  base::RunLoop().RunUntilIdle();
  ResetExpectedCallbackParams();

  EXPECT_FALSE(socket_watcher.ShouldNotifyUpdatedRTT());

  tick_clock.Advance(base::TimeDelta::FromMilliseconds(1000));
  // Minimum interval between consecutive notifications is 2000 msec.
  EXPECT_FALSE(socket_watcher.ShouldNotifyUpdatedRTT());

  // Advance the clock by 1000 msec more so that the current time is at least
  // 2000 msec more than the last time |socket_watcher| received a notification.
  tick_clock.Advance(base::TimeDelta::FromMilliseconds(1000));
  EXPECT_TRUE(socket_watcher.ShouldNotifyUpdatedRTT());
}

TEST_F(NetworkQualitySocketWatcherTest, PrivateAddressRTTNotNotified) {
  base::SimpleTestTickClock tick_clock;
  tick_clock.SetNowTicks(base::TimeTicks::Now());

  const struct {
    std::string ip_address;
    bool expect_should_notify_rtt;
  } tests[] = {
      {"157.0.0.1", true},    {"127.0.0.1", false},
      {"192.168.0.1", false}, {"::1", false},
      {"0.0.0.0", false},     {"2607:f8b0:4006:819::200e", true},
  };

  for (const auto& test : tests) {
    IPAddressList ip_list;
    IPAddress ip_address;
    ASSERT_TRUE(ip_address.AssignFromIPLiteral(test.ip_address));
    ip_list.push_back(ip_address);
    AddressList address_list =
        AddressList::CreateFromIPAddressList(ip_list, "canonical.example.com");

    SocketWatcher socket_watcher(
        SocketPerformanceWatcherFactory::PROTOCOL_QUIC, address_list,
        base::TimeDelta::FromMilliseconds(2000), false,
        base::ThreadTaskRunnerHandle::Get(), base::Bind(OnUpdatedRTTAvailable),
        &tick_clock);

    EXPECT_EQ(test.expect_should_notify_rtt,
              socket_watcher.ShouldNotifyUpdatedRTT());
    socket_watcher.OnUpdatedRTTAvailable(base::TimeDelta::FromSeconds(10));
    base::RunLoop().RunUntilIdle();
    ResetExpectedCallbackParams();

    EXPECT_FALSE(socket_watcher.ShouldNotifyUpdatedRTT());
  }
}

TEST_F(NetworkQualitySocketWatcherTest, IPv4SubnetComputedCorrectly) {
  base::SimpleTestTickClock tick_clock;
  tick_clock.SetNowTicks(base::TimeTicks::Now());
  const struct {
    std::string ip_address;
    uint64_t subnet_id;
  } tests[] = {
      {"112.112.112.100", 0x0000000000707070UL},  // IPv4.
      {"112.112.112.250", 0x0000000000707070UL},
      {"0:0:0:0:0:ffff:7070:7064",
       0x0000000000707070UL},  // 112.112.112.100 embedded in IPv6.
      {"0:0:0:0:0:ffff:7070:70fa",
       0x0000000000707070UL},  // 112.112.112.250 embedded in IPv6.
      {"2001:0db8:85a3:0000:0000:8a2e:0370:7334",
       0x000020010db885a3UL},                                 // IPv6.
      {"2001:db8:85a3::8a2e:370:7334", 0x000020010db885a3UL}  // Shortened IPv6.
  };

  for (const auto& test : tests) {
    IPAddressList ip_list;
    IPAddress ip_address;
    ASSERT_TRUE(ip_address.AssignFromIPLiteral(test.ip_address));
    ip_list.push_back(ip_address);
    AddressList address_list =
        AddressList::CreateFromIPAddressList(ip_list, "canonical.example.com");

    SocketWatcher socket_watcher(
        SocketPerformanceWatcherFactory::PROTOCOL_QUIC, address_list,
        base::TimeDelta::FromMilliseconds(2000), false,
        base::ThreadTaskRunnerHandle::Get(),
        base::Bind(OnUpdatedRTTAvailableStoreParams), &tick_clock);
    EXPECT_TRUE(socket_watcher.ShouldNotifyUpdatedRTT());
    socket_watcher.OnUpdatedRTTAvailable(base::TimeDelta::FromSeconds(10));
    base::RunLoop().RunUntilIdle();
    VerifyCallbackParams(base::TimeDelta::FromSeconds(10), test.subnet_id);
    EXPECT_FALSE(socket_watcher.ShouldNotifyUpdatedRTT());
  }
}

}  // namespace

}  // namespace internal

}  // namespace nqe

}  // namespace net
