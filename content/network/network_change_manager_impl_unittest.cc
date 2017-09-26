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
      : connection_type_(mojom::ConnectionType::CONNECTION_UNKNOWN),
        connection_subtype_(mojom::ConnectionSubtype::SUBTYPE_UNKNOWN),
        binding_(this) {
    mojom::NetworkChangeManagerRequest request(
        mojo::MakeRequest(&manager_ptr_));
    network_service->GetNetworkChangeManager(std::move(request));

    mojom::NetworkChangeManagerClientPtr client_ptr;
    mojom::NetworkChangeManagerClientRequest client_request(
        mojo::MakeRequest(&client_ptr));
    binding_.Bind(std::move(client_request));
    manager_ptr_->RequestNotification(std::move(client_ptr));
  }

  ~TestNetworkChangeManagerClient() override {}

  // NetworkChangeManagerClient implementation:
  void OnNetworkChanged(mojom::ConnectionType type,
                        mojom::ConnectionSubtype subtype) override {
    connection_type_ = type;
    connection_subtype_ = subtype;
    run_loop_.Quit();
  }

  void WaitForNotification() { run_loop_.Run(); }

  mojom::ConnectionType connection_type() { return connection_type_; }
  mojom::ConnectionSubtype connection_subtype() { return connection_subtype_; }

 private:
  base::RunLoop run_loop_;
  mojom::ConnectionType connection_type_;
  mojom::ConnectionSubtype connection_subtype_;
  mojom::NetworkChangeManagerPtr manager_ptr_;
  mojo::Binding<mojom::NetworkChangeManagerClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(TestNetworkChangeManagerClient);
};

}  // namespace

class NetworkChangeManagerImplTest : public testing::Test {
 public:
  NetworkChangeManagerImplTest()
      : network_change_notifier_(net::NetworkChangeNotifier::Create()),
        network_service_(NetworkServiceImpl::CreateForTesting()) {
    network_change_manager_client_ =
        std::make_unique<TestNetworkChangeManagerClient>(
            network_service_.get());
  }

  ~NetworkChangeManagerImplTest() override {}

  TestNetworkChangeManagerClient* network_change_manager_client() {
    return network_change_manager_client_.get();
  }

 private:
  base::MessageLoop loop_;
  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier_;
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
}

}  // namespace content
