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
        connection_type_initialized_(false),
        connection_type_(mojom::ConnectionType::CONNECTION_UNKNOWN) {
    tracker_->AddNetworkConnectionObserver(this);
  }

  ~TestNetworkConnectionObserver() override {
    tracker_->RemoveNetworkConnectionObserver(this);
  }

  // Helper to synchronously get connection type from NetworkConnectionTracker.
  mojom::ConnectionType GetConnectionTypeSync() {
    mojom::ConnectionType type;
    base::RunLoop run_loop;
    bool sync = tracker_->GetConnectionType(
        &type,
        base::Bind(&TestNetworkConnectionObserver::GetConnectionTypeCallback,
                   &run_loop, &type));
    if (!sync)
      run_loop.Run();
    return type;
  }

  // NetworkConnectionObserver implementation:
  void OnConnectionChanged(mojom::ConnectionType type) override {
    EXPECT_EQ(type, GetConnectionTypeSync());

    num_notifications_++;
    connection_type_ = type;
    run_loop_->Quit();
  }

  size_t num_notifications() const { return num_notifications_; }
  void WaitForNotification() {
    run_loop_->Run();
    run_loop_.reset(new base::RunLoop());
  }

  mojom::ConnectionType GetConnectionType() {
    if (!connection_type_initialized_)
      connection_type_ = GetConnectionTypeSync();
    return connection_type_;
  }

 private:
  static void GetConnectionTypeCallback(base::RunLoop* run_loop,
                                        mojom::ConnectionType* out,
                                        mojom::ConnectionType type) {
    *out = type;
    run_loop->Quit();
  }

  size_t num_notifications_;
  NetworkConnectionTracker* tracker_;
  std::unique_ptr<base::RunLoop> run_loop_;
  bool connection_type_initialized_;
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
            network_connection_observer()->GetConnectionType());

  // Simulate a network change.
  SimulateConnectionTypeChange(
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_3G);

  network_connection_observer()->WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_3G,
            network_connection_observer()->GetConnectionType());
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
            network_connection_observer2->GetConnectionType());
  network_connection_observer()->WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_WIFI,
            network_connection_observer()->GetConnectionType());
  base::RunLoop().RunUntilIdle();

  network_connection_observer2.reset();

  // Simulate an another network change.
  SimulateConnectionTypeChange(
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_2G);
  network_connection_observer()->WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_2G,
            network_connection_observer()->GetConnectionType());
  EXPECT_EQ(2u, network_connection_observer()->num_notifications());
}

class GetConnectionTypeTest : public testing::Test {
 public:
  GetConnectionTypeTest() {}
  ~GetConnectionTypeTest() override {}

  void OnGetConnectionType(mojom::ConnectionType* out,
                           mojom::ConnectionType type) {
    num_get_connection_type_++;
    *out = type;
    if (num_get_connection_type_ == desired_num_get_connection_type_)
      run_loop_.Quit();
  }

  void WaitForNumGetConnectionType(size_t num_get_connection_type) {
    desired_num_get_connection_type_ = num_get_connection_type;
    run_loop_.Run();
  }

 private:
  base::MessageLoop loop_;
  base::RunLoop run_loop_;
  size_t num_get_connection_type_ = 0;
  size_t desired_num_get_connection_type_ = 0;

  DISALLOW_COPY_AND_ASSIGN(GetConnectionTypeTest);
};

TEST_F(GetConnectionTypeTest, GetConnectionType) {
  net::test::MockNetworkChangeNotifier mock_network_change_notifier;
  mock_network_change_notifier.SetConnectionType(
      net::NetworkChangeNotifier::ConnectionType::CONNECTION_3G);
  std::unique_ptr<NetworkService> network_service = NetworkService::Create();
  NetworkConnectionTracker network_connection_tracker(network_service.get());

  mojom::ConnectionType connection_type1;
  mojom::ConnectionType connection_type2;
  // These two GetConnectionType() will finish asynchonously because network
  // service is not yet set up.
  EXPECT_FALSE(network_connection_tracker.GetConnectionType(
      &connection_type1,
      base::Bind(&GetConnectionTypeTest::OnGetConnectionType,
                 base::Unretained(this), &connection_type1)));
  EXPECT_FALSE(network_connection_tracker.GetConnectionType(
      &connection_type2,
      base::Bind(&GetConnectionTypeTest::OnGetConnectionType,
                 base::Unretained(this), &connection_type2)));

  WaitForNumGetConnectionType(2);
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_3G, connection_type1);
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_3G, connection_type2);

  mojom::ConnectionType connection_type3;
  // This GetConnectionType() should finish synchronously.
  EXPECT_TRUE(network_connection_tracker.GetConnectionType(
      &connection_type3,
      base::Bind(&GetConnectionTypeTest::OnGetConnectionType,
                 base::Unretained(this), &connection_type3)));
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_3G, connection_type3);
}

}  // namespace content
