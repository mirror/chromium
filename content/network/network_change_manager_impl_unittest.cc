// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_change_manager_impl.h"

#include <algorithm>

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
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
        connection_subtype_(mojom::ConnectionSubtype::SUBTYPE_UNKNOWN),
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
  void OnNetworkChanged(mojom::ConnectionType type,
                        mojom::ConnectionSubtype subtype) override {
    num_notifications_++;
    connection_type_ = type;
    connection_subtype_ = subtype;
    run_loop_->Quit();
  }

  size_t num_notifications() const { return num_notifications_; }
  void WaitForNotification() {
    run_loop_->Run();
    run_loop_.reset(new base::RunLoop());
  }

  mojom::ConnectionType connection_type() { return connection_type_; }
  mojom::ConnectionSubtype connection_subtype() { return connection_subtype_; }

 private:
  size_t num_notifications_;
  std::unique_ptr<base::RunLoop> run_loop_;
  mojom::ConnectionType connection_type_;
  mojom::ConnectionSubtype connection_subtype_;
  mojo::Binding<mojom::NetworkChangeManagerClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestNetworkChangeManagerClient);
};

}  // namespace

class NetworkChangeManagerImplTest : public testing::Test {
 public:
  NetworkChangeManagerImplTest()
      : network_service_(NetworkServiceImpl::CreateForTesting()) {
    network_change_manager_client_ =
        std::make_unique<TestNetworkChangeManagerClient>(
            network_service_.get());
  }

  ~NetworkChangeManagerImplTest() override {}

  TestNetworkChangeManagerClient* network_change_manager_client() {
    return network_change_manager_client_.get();
  }

  NetworkServiceImpl* network_service() { return network_service_.get(); }

 private:
  base::MessageLoop loop_;
  std::unique_ptr<NetworkServiceImpl> network_service_;
  std::unique_ptr<TestNetworkChangeManagerClient>
      network_change_manager_client_;

  DISALLOW_COPY_AND_ASSIGN(NetworkChangeManagerImplTest);
};

TEST_F(NetworkChangeManagerImplTest, ClientNotified) {
  ASSERT_EQ(mojom::ConnectionType::CONNECTION_UNKNOWN,
            network_change_manager_client()->connection_type());
  ASSERT_EQ(mojom::ConnectionSubtype::SUBTYPE_UNKNOWN,
            network_change_manager_client()->connection_subtype());

  // Simulate a network change.
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_3G);
  network_change_manager_client()->WaitForNotification();
  ASSERT_EQ(mojom::ConnectionType::CONNECTION_3G,
            network_change_manager_client()->connection_type());
  ASSERT_EQ(mojom::ConnectionSubtype::SUBTYPE_UNKNOWN,
            network_change_manager_client()->connection_subtype());
  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(1u, network_change_manager_client()->num_notifications());
}

TEST_F(NetworkChangeManagerImplTest, OneClientPipeBroken) {
  auto network_change_manager_client2 =
      std::make_unique<TestNetworkChangeManagerClient>(network_service());

  // Simulate a network change.
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_WIFI);

  network_change_manager_client2->WaitForNotification();
  network_change_manager_client()->WaitForNotification();
  ASSERT_EQ(mojom::ConnectionType::CONNECTION_WIFI,
            network_change_manager_client2->connection_type());
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(2u, network_service()
                    ->GetNetworkChangeManagerForTesting()
                    ->GetClientsForTesting()
                    .size());

  ASSERT_EQ(1u, network_change_manager_client()->num_notifications());
  ASSERT_EQ(1u, network_change_manager_client2->num_notifications());
  network_change_manager_client2.reset();

  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(1u, network_service()
                    ->GetNetworkChangeManagerForTesting()
                    ->GetClientsForTesting()
                    .size());
  // Simulate a second network change, and the remaining client should be
  // notified.
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_2G);

  network_change_manager_client()->WaitForNotification();
  EXPECT_EQ(mojom::ConnectionType::CONNECTION_2G,
            network_change_manager_client()->connection_type());
  EXPECT_EQ(2u, network_change_manager_client()->num_notifications());
}

}  // namespace content
