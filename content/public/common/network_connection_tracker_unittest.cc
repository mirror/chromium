// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/network_connection_tracker.h"

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "content/public/common/network_change_manager.mojom.h"
#include "content/public/network/network_service.h"
#include "net/base/mock_network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

class TestNetworkConnectionObserver
    : public NetworkConnectionTracker::NetworkConnectionObserver {
 public:
  explicit TestNetworkConnectionObserver(NetworkConnectionTracker* tracker)
      : num_notifications_(0),
        tracker_(tracker),
        run_loop_(std::make_unique<base::RunLoop>()),
        connection_type_(tracker_->GetConnectionType()) {
    tracker_->AddNetworkConnectionObserver(this);
  }

  ~TestNetworkConnectionObserver() override {
    tracker_->RemoveNetworkConnectionObserver(this);
  }

  // NetworkConnectionObserver implementation:
  void OnConnectionChanged(mojom::ConnectionType type) override {
    EXPECT_EQ(type, tracker_->GetConnectionType());

    num_notifications_++;
    connection_type_ = type;
    run_loop_->Quit();
  }

  size_t num_notifications() const { return num_notifications_; }
  void WaitForNotification() {
    run_loop_->Run();
    run_loop_.reset(new base::RunLoop());
  }

  mojom::ConnectionType connection_type() const { return connection_type_; }

 private:
  size_t num_notifications_;
  NetworkConnectionTracker* tracker_;
  std::unique_ptr<base::RunLoop> run_loop_;
  mojom::ConnectionType connection_type_;

  DISALLOW_COPY_AND_ASSIGN(TestNetworkConnectionObserver);
};

}  // namespace

class NetworkConnectionTrackerTest : public testing::Test {
 public:
  NetworkConnectionTrackerTest()
      : network_service_(NetworkService::Create()),
        network_connection_tracker_(network_service_.get()),
        network_connection_observer_(&network_connection_tracker_) {}

  ~NetworkConnectionTrackerTest() override {}

  NetworkConnectionTracker* network_connection_tracker() {
    return &network_connection_tracker_;
  }

  TestNetworkConnectionObserver* network_connection_observer() {
    return &network_connection_observer_;
  }

  void SimulateConnectionTypeChange(
      net::NetworkChangeNotifier::ConnectionType type) {
    mock_network_change_notifier_.NotifyObserversOfNetworkChangeForTests(type);
  }

 private:
  base::MessageLoop loop_;
  net::test::MockNetworkChangeNotifier mock_network_change_notifier_;
  std::unique_ptr<NetworkService> network_service_;
  NetworkConnectionTracker network_connection_tracker_;
  TestNetworkConnectionObserver network_connection_observer_;

  DISALLOW_COPY_AND_ASSIGN(NetworkConnectionTrackerTest);
};

TEST_F(NetworkConnectionTrackerTest, ObserverNotified) {
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_UNKNOWN,
            network_connection_observer()->connection_type());

  // Simulate a network change.
  SimulateConnectionTypeChange(
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_3G);

  network_connection_observer()->WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_3G,
            network_connection_observer()->connection_type());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1u, network_connection_observer()->num_notifications());
}

TEST_F(NetworkConnectionTrackerTest, UnregisteredObserverNotNotified) {
  auto network_connection_observer2 =
      std::make_unique<TestNetworkConnectionObserver>(
          network_connection_tracker());

  // Simulate a network change.
  SimulateConnectionTypeChange(
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_WIFI);

  network_connection_observer2->WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_WIFI,
            network_connection_observer2->connection_type());
  network_connection_observer()->WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_WIFI,
            network_connection_observer()->connection_type());
  base::RunLoop().RunUntilIdle();

  network_connection_observer2.reset();

  // Simulate an another network change.
  SimulateConnectionTypeChange(
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_2G);
  network_connection_observer()->WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_2G,
            network_connection_observer()->connection_type());
  EXPECT_EQ(2u, network_connection_observer()->num_notifications());
}

}  // namespace content
