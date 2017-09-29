// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/network_connection_tracker.h"

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "content/network/network_service_impl.h"
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
    num_notifications_++;
    connection_type_ = type;
    run_loop_->Quit();
  }

  size_t num_notifications() const { return num_notifications_; }
  void WaitForNotification() {
    run_loop_->Run();
    run_loop_.reset(new base::RunLoop());
  }

  mojom::ConnectionType connection_type() { return connection_type_; }

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
      : network_service_(NetworkServiceImpl::CreateForTesting()),
        network_connection_tracker_(
            std::make_unique<NetworkConnectionTracker>(network_service_.get())),
        network_connection_observer_(
            std::make_unique<TestNetworkConnectionObserver>(
                network_connection_tracker_.get())) {}

  ~NetworkConnectionTrackerTest() override {}

  NetworkConnectionTracker* network_connection_tracker() {
    return network_connection_tracker_.get();
  }

  TestNetworkConnectionObserver* network_connection_observer() {
    return network_connection_observer_.get();
  }

  NetworkServiceImpl* network_service() { return network_service_.get(); }

 private:
  base::MessageLoop loop_;
  std::unique_ptr<NetworkServiceImpl> network_service_;
  std::unique_ptr<NetworkConnectionTracker> network_connection_tracker_;
  std::unique_ptr<TestNetworkConnectionObserver> network_connection_observer_;

  DISALLOW_COPY_AND_ASSIGN(NetworkConnectionTrackerTest);
};

TEST_F(NetworkConnectionTrackerTest, ObserverNotified) {
  ASSERT_EQ(mojom::ConnectionType::CONNECTION_UNKNOWN,
            network_connection_observer()->connection_type());

  // Simulate a network change.
  network_service()
      ->GetNetworkChangeManagerForTesting()
      ->SimulateConnectionTypeChangeForTesting(
          mojom::ConnectionType::CONNECTION_3G);

  network_connection_observer()->WaitForNotification();
  ASSERT_EQ(mojom::ConnectionType::CONNECTION_3G,
            network_connection_observer()->connection_type());
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(1u, network_connection_observer()->num_notifications());
}

TEST_F(NetworkConnectionTrackerTest, UnregisteredObserverNotNotified) {
  auto network_connection_observer2 =
      std::make_unique<TestNetworkConnectionObserver>(
          network_connection_tracker());

  // Simulate a network change.
  network_service()
      ->GetNetworkChangeManagerForTesting()
      ->SimulateConnectionTypeChangeForTesting(
          mojom::ConnectionType::CONNECTION_WIFI);

  network_connection_observer2->WaitForNotification();
  ASSERT_EQ(mojom::ConnectionType::CONNECTION_WIFI,
            network_connection_observer2->connection_type());
  network_connection_observer()->WaitForNotification();
  ASSERT_EQ(mojom::ConnectionType::CONNECTION_WIFI,
            network_connection_observer()->connection_type());
  base::RunLoop().RunUntilIdle();

  network_connection_observer2.reset();

  // Simulate an another network change.
  network_service()
      ->GetNetworkChangeManagerForTesting()
      ->SimulateConnectionTypeChangeForTesting(
          mojom::ConnectionType::CONNECTION_2G);
  network_connection_observer()->WaitForNotification();
  ASSERT_EQ(mojom::ConnectionType::CONNECTION_2G,
            network_connection_observer()->connection_type());
  ASSERT_EQ(2u, network_connection_observer()->num_notifications());
}

}  // namespace content
