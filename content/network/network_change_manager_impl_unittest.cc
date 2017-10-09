// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_change_manager_impl.h"

#include <algorithm>
#include <utility>

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "content/network/network_service_impl.h"
#include "content/public/common/network_change_manager.mojom.h"
#include "net/base/network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

class TestNetworkChangeManagerClient
    : public mojom::NetworkChangeManagerClient {
 public:
  explicit TestNetworkChangeManagerClient(
      mojom::NetworkService* network_service)
      : num_notifications_(0),
        run_loop_(std::make_unique<base::RunLoop>()),
        connection_type_(mojom::ConnectionType::CONNECTION_UNKNOWN),
        binding_(this) {
    mojom::NetworkChangeManagerPtr manager_ptr;
    mojom::NetworkChangeManagerRequest request(mojo::MakeRequest(&manager_ptr));
    network_service->GetNetworkChangeManager(std::move(request));

    mojom::NetworkChangeManagerClientPtr client_ptr;
    mojom::NetworkChangeManagerClientRequest client_request(
        mojo::MakeRequest(&client_ptr));
    binding_.Bind(std::move(client_request));
    manager_ptr->RequestNotifications(std::move(client_ptr));
  }

  ~TestNetworkChangeManagerClient() override {}

  // NetworkChangeManagerClient implementation:
  void OnNetworkChanged(mojom::ConnectionType type) override {
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
  std::unique_ptr<base::RunLoop> run_loop_;
  mojom::ConnectionType connection_type_;
  mojo::Binding<mojom::NetworkChangeManagerClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestNetworkChangeManagerClient);
};

}  // namespace

class NetworkChangeManagerImplTest : public testing::Test {
 public:
  NetworkChangeManagerImplTest()
      : network_service_(NetworkServiceImpl::CreateForTesting()),
        network_change_manager_client_(network_service_.get()) {}

  ~NetworkChangeManagerImplTest() override {}

  TestNetworkChangeManagerClient* network_change_manager_client() {
    return &network_change_manager_client_;
  }

  NetworkServiceImpl* network_service() const { return network_service_.get(); }

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::unique_ptr<NetworkServiceImpl> network_service_;
  TestNetworkChangeManagerClient network_change_manager_client_;

  DISALLOW_COPY_AND_ASSIGN(NetworkChangeManagerImplTest);
};

// ChromeOS has a different code path to set up net::NetworkChangeNotifier.
#if defined(OS_CHROMEOS)
#define MAYBE_ClientNotified DISABLED_ClientNotified
#else
#define MAYBE_ClientNotified ClientNotified
#endif
TEST_F(NetworkChangeManagerImplTest, MAYBE_ClientNotified) {
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_UNKNOWN,
            network_change_manager_client()->connection_type());

  // Receives the current connection type.
  network_change_manager_client()->WaitForNotification();
  EXPECT_NE(mojom::ConnectionType::CONNECTION_UNKNOWN,
            network_change_manager_client()->connection_type());

  // Simulate a new network change.
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_3G);
  network_change_manager_client()->WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_3G,
            network_change_manager_client()->connection_type());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(2u, network_change_manager_client()->num_notifications());
}

// ChromeOS has a different code path to set up net::NetworkChangeNotifier.
#if defined(OS_CHROMEOS)
#define MAYBE_OneClientPipeBroken DISABLED_OneClientPipeBroken
#else
#define MAYBE_OneClientPipeBroken OneClientPipeBroken
#endif
TEST_F(NetworkChangeManagerImplTest, MAYBE_OneClientPipeBroken) {
  auto network_change_manager_client2 =
      std::make_unique<TestNetworkChangeManagerClient>(network_service());

  // Receives initial connecion type.
  network_change_manager_client2->WaitForNotification();
  network_change_manager_client()->WaitForNotification();
  EXPECT_EQ(1u, network_change_manager_client()->num_notifications());
  EXPECT_EQ(1u, network_change_manager_client2->num_notifications());

  // Simulate a network change.
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);

  network_change_manager_client2->WaitForNotification();
  network_change_manager_client()->WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_WIFI,
            network_change_manager_client2->connection_type());
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ(2u, network_service()
                    ->network_change_manager_for_testing()
                    .GetNumClientsForTesting());

  EXPECT_EQ(2u, network_change_manager_client()->num_notifications());
  EXPECT_EQ(2u, network_change_manager_client2->num_notifications());
  network_change_manager_client2.reset();

  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(1u, network_service()
                    ->network_change_manager_for_testing()
                    .GetNumClientsForTesting());
  // Simulate a second network change, and the remaining client should be
  // notified.
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_2G);

  network_change_manager_client()->WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_2G,
            network_change_manager_client()->connection_type());
  EXPECT_EQ(3u, network_change_manager_client()->num_notifications());
}

// ChromeOS has a different code path to set up net::NetworkChangeNotifier.
#if defined(OS_CHROMEOS)
#define MAYBE_NewClientReceivesCurrentType DISABLED_NewClientReceivesCurrentType
#else
#define MAYBE_NewClientReceivesCurrentType NewClientReceivesCurrentType
#endif
TEST_F(NetworkChangeManagerImplTest, MAYBE_NewClientReceivesCurrentType) {
  // Simulate a network change.
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_BLUETOOTH);

  network_change_manager_client()->WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_BLUETOOTH,
            network_change_manager_client()->connection_type());
  base::RunLoop().RunUntilIdle();

  // Register a new client after the network change and it should receive the
  // up-to-date connection type.
  auto network_change_manager_client2 =
      std::make_unique<TestNetworkChangeManagerClient>(network_service());
  network_change_manager_client2->WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_BLUETOOTH,
            network_change_manager_client2->connection_type());
}

TEST(NetworkChangeConnectionTypeTest, ConnectionTypeEnumMatch) {
  for (int typeInt = net::NetworkChangeNotifier::CONNECTION_UNKNOWN;
       typeInt != net::NetworkChangeNotifier::CONNECTION_LAST; typeInt++) {
    mojom::ConnectionType mojoType = mojom::ConnectionType(typeInt);
    switch (typeInt) {
      case net::NetworkChangeNotifier::CONNECTION_UNKNOWN:
        EXPECT_EQ(mojom::ConnectionType::CONNECTION_UNKNOWN, mojoType);
        break;
      case net::NetworkChangeNotifier::CONNECTION_ETHERNET:
        EXPECT_EQ(mojom::ConnectionType::CONNECTION_ETHERNET, mojoType);
        break;
      case net::NetworkChangeNotifier::CONNECTION_WIFI:
        EXPECT_EQ(mojom::ConnectionType::CONNECTION_WIFI, mojoType);
        break;
      case net::NetworkChangeNotifier::CONNECTION_2G:
        EXPECT_EQ(mojom::ConnectionType::CONNECTION_2G, mojoType);
        break;
      case net::NetworkChangeNotifier::CONNECTION_3G:
        EXPECT_EQ(mojom::ConnectionType::CONNECTION_3G, mojoType);
        break;
      case net::NetworkChangeNotifier::CONNECTION_4G:
        EXPECT_EQ(mojom::ConnectionType::CONNECTION_4G, mojoType);
        break;
      case net::NetworkChangeNotifier::CONNECTION_NONE:
        EXPECT_EQ(mojom::ConnectionType::CONNECTION_NONE, mojoType);
        break;
      case net::NetworkChangeNotifier::CONNECTION_BLUETOOTH:
        EXPECT_EQ(mojom::ConnectionType::CONNECTION_BLUETOOTH, mojoType);
        EXPECT_EQ(mojom::ConnectionType::CONNECTION_LAST, mojoType);
        break;
    }
  }
}

}  // namespace content
