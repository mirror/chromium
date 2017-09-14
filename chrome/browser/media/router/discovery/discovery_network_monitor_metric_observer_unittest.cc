// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/discovery/discovery_network_monitor_metric_observer.h"

#include "base/test/simple_test_tick_clock.h"
#include "base/timer/mock_timer.h"
#include "net/base/mock_network_change_notifier.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media_router {

std::ostream& operator<<(
    std::ostream& os,
    DiscoveryNetworkMonitorConnectionType connection_type) {
  switch (connection_type) {
    case DiscoveryNetworkMonitorConnectionType::kWifi:
      os << "kWifi";
      break;
    case DiscoveryNetworkMonitorConnectionType::kEthernet:
      os << "kEthernet";
      break;
    case DiscoveryNetworkMonitorConnectionType::kUnknownReportedAsWifi:
      os << "kUnknownReportedAsWifi";
      break;
    case DiscoveryNetworkMonitorConnectionType::kUnknownReportedAsEthernet:
      os << "kUnknownReportedAsEthernet";
      break;
    case DiscoveryNetworkMonitorConnectionType::kUnknownReportedAsOther:
      os << "kUnknownReportedAsOther";
      break;
    case DiscoveryNetworkMonitorConnectionType::kUnknown:
      os << "kUnknown";
      break;
    case DiscoveryNetworkMonitorConnectionType::kDisconnected:
      os << "kDisconnected";
      break;
    default:
      os << "Bad DiscoveryNetworkMonitorConnectionType value";
      break;
  }
  return os;
}

namespace {

using ::testing::_;

class MockMetrics : public DiscoveryNetworkMonitorMetrics {
 public:
  MOCK_METHOD1(RecordTimeBetweenNetworkChangeEvents, void(base::TimeDelta));
  MOCK_METHOD1(RecordConnectionType,
               void(DiscoveryNetworkMonitorConnectionType));
};

class MockNetworkChangeNotifier : public net::NetworkChangeNotifier {
 public:
  ConnectionType GetCurrentConnectionType() const override {
    return connection_type_;
  }

  void SetConnectionType(ConnectionType connection_type) {
    connection_type_ = connection_type;
  }

 private:
  ConnectionType connection_type_;
};

class DiscoveryNetworkMonitorMetricObserverTest : public ::testing::Test {
 public:
  DiscoveryNetworkMonitorMetricObserverTest()
      : mock_network_change_notifier_(
            base::MakeUnique<MockNetworkChangeNotifier>()),
        clock_(base::MakeUnique<base::SimpleTestTickClock>()),
        test_clock_(clock_.get()),
        disconnect_timer_(false, false),
        metrics_(base::MakeUnique<MockMetrics>()),
        mock_metrics_(metrics_.get()),
        metric_observer_(std::move(clock_), std::move(metrics_)) {
    metric_observer_.SetTimerForTest(&disconnect_timer_);
  }

 protected:
  base::TimeTicks start_ticks_;
  base::TimeDelta time_advance_ = base::TimeDelta::FromSeconds(1);
  std::unique_ptr<MockNetworkChangeNotifier> mock_network_change_notifier_;
  std::unique_ptr<base::SimpleTestTickClock> clock_;
  base::SimpleTestTickClock* test_clock_;
  base::MockTimer disconnect_timer_;
  std::unique_ptr<MockMetrics> metrics_;
  MockMetrics* mock_metrics_;
  DiscoveryNetworkMonitorMetricObserver metric_observer_;
};

}  // namespace

TEST_F(DiscoveryNetworkMonitorMetricObserverTest, RecordsFirstGoodNetworkWifi) {
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  EXPECT_CALL(*mock_metrics_, RecordTimeBetweenNetworkChangeEvents(_)).Times(0);
  EXPECT_CALL(
      *mock_metrics_,
      RecordConnectionType(DiscoveryNetworkMonitorConnectionType::kWifi));
  metric_observer_.OnNetworksChanged("network1");
}

TEST_F(DiscoveryNetworkMonitorMetricObserverTest,
       RecordsFirstGoodNetworkEthernet) {
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  EXPECT_CALL(*mock_metrics_, RecordTimeBetweenNetworkChangeEvents(_)).Times(0);
  EXPECT_CALL(
      *mock_metrics_,
      RecordConnectionType(DiscoveryNetworkMonitorConnectionType::kEthernet));
  metric_observer_.OnNetworksChanged("network1");
}

TEST_F(DiscoveryNetworkMonitorMetricObserverTest,
       RecordsFirstGoodNetworkUnknownWifi) {
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_WIFI);
  EXPECT_CALL(*mock_metrics_, RecordTimeBetweenNetworkChangeEvents(_)).Times(0);
  EXPECT_CALL(
      *mock_metrics_,
      RecordConnectionType(
          DiscoveryNetworkMonitorConnectionType::kUnknownReportedAsWifi));
  metric_observer_.OnNetworksChanged(
      DiscoveryNetworkMonitor::kNetworkIdUnknown);
}

TEST_F(DiscoveryNetworkMonitorMetricObserverTest,
       RecordsFirstGoodNetworkUnknownEthernet) {
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  EXPECT_CALL(*mock_metrics_, RecordTimeBetweenNetworkChangeEvents(_)).Times(0);
  EXPECT_CALL(
      *mock_metrics_,
      RecordConnectionType(
          DiscoveryNetworkMonitorConnectionType::kUnknownReportedAsEthernet));
  metric_observer_.OnNetworksChanged(
      DiscoveryNetworkMonitor::kNetworkIdUnknown);
}

TEST_F(DiscoveryNetworkMonitorMetricObserverTest,
       RecordsFirstGoodNetworkUnknownOther) {
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_4G);
  EXPECT_CALL(*mock_metrics_, RecordTimeBetweenNetworkChangeEvents(_)).Times(0);
  EXPECT_CALL(
      *mock_metrics_,
      RecordConnectionType(
          DiscoveryNetworkMonitorConnectionType::kUnknownReportedAsOther));
  metric_observer_.OnNetworksChanged(
      DiscoveryNetworkMonitor::kNetworkIdUnknown);
}

TEST_F(DiscoveryNetworkMonitorMetricObserverTest,
       RecordsFirstGoodNetworkUnknown) {
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN);
  EXPECT_CALL(*mock_metrics_, RecordTimeBetweenNetworkChangeEvents(_)).Times(0);
  EXPECT_CALL(
      *mock_metrics_,
      RecordConnectionType(DiscoveryNetworkMonitorConnectionType::kUnknown));
  metric_observer_.OnNetworksChanged(
      DiscoveryNetworkMonitor::kNetworkIdUnknown);
}

TEST_F(DiscoveryNetworkMonitorMetricObserverTest,
       RecordsFirstGoodNetworkDisconnected) {
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  EXPECT_CALL(*mock_metrics_, RecordTimeBetweenNetworkChangeEvents(_)).Times(0);
  EXPECT_CALL(*mock_metrics_,
              RecordConnectionType(
                  DiscoveryNetworkMonitorConnectionType::kDisconnected));
  metric_observer_.OnNetworksChanged(
      DiscoveryNetworkMonitor::kNetworkIdDisconnected);
}

TEST_F(DiscoveryNetworkMonitorMetricObserverTest,
       DoesntRecordEphemeralDisconnectedState) {
  EXPECT_CALL(*mock_metrics_, RecordTimeBetweenNetworkChangeEvents(_)).Times(0);
  EXPECT_CALL(
      *mock_metrics_,
      RecordConnectionType(DiscoveryNetworkMonitorConnectionType::kEthernet));
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  metric_observer_.OnNetworksChanged("network1");

  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  metric_observer_.OnNetworksChanged(
      DiscoveryNetworkMonitor::kNetworkIdDisconnected);
  ASSERT_TRUE(disconnect_timer_.IsRunning());

  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  EXPECT_CALL(*mock_metrics_, RecordTimeBetweenNetworkChangeEvents(_));
  EXPECT_CALL(
      *mock_metrics_,
      RecordConnectionType(DiscoveryNetworkMonitorConnectionType::kEthernet));
  metric_observer_.OnNetworksChanged("network2");

  ASSERT_FALSE(disconnect_timer_.IsRunning());
}

TEST_F(DiscoveryNetworkMonitorMetricObserverTest,
       RecordsTimeChangeBetweenConnectionTypeEvents) {
  EXPECT_CALL(*mock_metrics_, RecordTimeBetweenNetworkChangeEvents(_)).Times(0);
  EXPECT_CALL(
      *mock_metrics_,
      RecordConnectionType(DiscoveryNetworkMonitorConnectionType::kEthernet));
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  metric_observer_.OnNetworksChanged("network1");

  test_clock_->Advance(time_advance_);
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  metric_observer_.OnNetworksChanged(
      DiscoveryNetworkMonitor::kNetworkIdDisconnected);
  ASSERT_TRUE(disconnect_timer_.IsRunning());

  test_clock_->Advance(time_advance_);
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  EXPECT_CALL(*mock_metrics_,
              RecordTimeBetweenNetworkChangeEvents(
                  (start_ticks_ + time_advance_ * 2) - start_ticks_));
  EXPECT_CALL(
      *mock_metrics_,
      RecordConnectionType(DiscoveryNetworkMonitorConnectionType::kEthernet));
  metric_observer_.OnNetworksChanged("network2");

  ASSERT_FALSE(disconnect_timer_.IsRunning());
}

TEST_F(DiscoveryNetworkMonitorMetricObserverTest,
       RecordChangeToDisconnectedState) {
  EXPECT_CALL(*mock_metrics_, RecordTimeBetweenNetworkChangeEvents(_)).Times(0);
  EXPECT_CALL(
      *mock_metrics_,
      RecordConnectionType(DiscoveryNetworkMonitorConnectionType::kEthernet));
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  metric_observer_.OnNetworksChanged("network1");

  test_clock_->Advance(time_advance_);
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  metric_observer_.OnNetworksChanged(
      DiscoveryNetworkMonitor::kNetworkIdDisconnected);
  ASSERT_TRUE(disconnect_timer_.IsRunning());

  test_clock_->Advance(time_advance_);
  EXPECT_CALL(*mock_metrics_,
              RecordTimeBetweenNetworkChangeEvents(
                  (start_ticks_ + time_advance_) - start_ticks_));
  EXPECT_CALL(*mock_metrics_,
              RecordConnectionType(
                  DiscoveryNetworkMonitorConnectionType::kDisconnected));

  ASSERT_TRUE(disconnect_timer_.IsRunning());
  disconnect_timer_.Fire();
}

TEST_F(DiscoveryNetworkMonitorMetricObserverTest,
       RecordChangeFromDisconnectedState) {
  EXPECT_CALL(*mock_metrics_, RecordTimeBetweenNetworkChangeEvents(_)).Times(0);
  EXPECT_CALL(
      *mock_metrics_,
      RecordConnectionType(DiscoveryNetworkMonitorConnectionType::kEthernet));
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  metric_observer_.OnNetworksChanged("network1");

  test_clock_->Advance(time_advance_);
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_NONE);
  metric_observer_.OnNetworksChanged(
      DiscoveryNetworkMonitor::kNetworkIdDisconnected);
  ASSERT_TRUE(disconnect_timer_.IsRunning());

  test_clock_->Advance(time_advance_);
  EXPECT_CALL(*mock_metrics_,
              RecordTimeBetweenNetworkChangeEvents(
                  (start_ticks_ + time_advance_) - start_ticks_));
  EXPECT_CALL(*mock_metrics_,
              RecordConnectionType(
                  DiscoveryNetworkMonitorConnectionType::kDisconnected));

  ASSERT_TRUE(disconnect_timer_.IsRunning());
  disconnect_timer_.Fire();

  test_clock_->Advance(time_advance_);
  EXPECT_CALL(*mock_metrics_, RecordTimeBetweenNetworkChangeEvents(
                                  (start_ticks_ + time_advance_ * 3) -
                                  (start_ticks_ + time_advance_)));
  EXPECT_CALL(
      *mock_metrics_,
      RecordConnectionType(DiscoveryNetworkMonitorConnectionType::kEthernet));
  mock_network_change_notifier_->SetConnectionType(
      net::NetworkChangeNotifier::CONNECTION_ETHERNET);
  metric_observer_.OnNetworksChanged("network1");
}

}  // namespace media_router
