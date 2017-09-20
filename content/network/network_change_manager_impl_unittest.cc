// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/network/network_change_manager_impl.h"

#include <algorithm>

#include "base/macros.h"
#include "base/run_loop.h"
#include "content/network/network_change_manager_client_impl.h"
#include "content/network/network_service_impl.h"
#include "content/public/common/network_change_manager.mojom.h"
#include "net/base/network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {

namespace {

class TestObserver
    : public NetworkChangeManagerClientImpl::NetworkChangeObserver {
 public:
  explicit TestObserver(
      NetworkChangeManagerClientImpl* network_change_manager_client)
      : network_change_manager_client_(network_change_manager_client) {
    network_change_manager_client_->AddNetworkChangeObserver(this);
  }
  ~TestObserver() override {
    network_change_manager_client_->RemoveNetworkChangeObserver(this);
  }

  void OnNetworkChanged(mojom::ConnectionType type) override {
    run_loop_.Quit();
  }

  void WaitForNotification() { run_loop_.Run(); }

 private:
  base::RunLoop run_loop_;
  NetworkChangeManagerClientImpl* network_change_manager_client_;

  DISALLOW_COPY_AND_ASSIGN(TestObserver);
};

}  // namespace

class NetworkChangeManagerImplTest : public testing::Test {
 public:
  NetworkChangeManagerImplTest()
      : network_change_notifier_(net::NetworkChangeNotifier::Create()),
        network_service_(NetworkServiceImpl::CreateForTesting()) {
    network_change_manager_client_ =
        std::make_unique<NetworkChangeManagerClientImpl>(
            network_service_.get());
  }

  ~NetworkChangeManagerImplTest() override {}

  mojom::NetworkService* network_service() { return network_service_.get(); }

  NetworkChangeManagerClientImpl* network_change_manager_client() {
    return network_change_manager_client_.get();
  }

 private:
  TestBrowserThreadBundle thread_bundle_;
  base::test::ScopedFeatureList feature_list_;
  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier_;
  std::unique_ptr<NetworkServiceImpl> network_service_;

  std::unique_ptr<NetworkChangeManagerClientImpl>
      network_change_manager_client_;

  DISALLOW_COPY_AND_ASSIGN(NetworkChangeManagerImplTest);
};

TEST_F(NetworkChangeManagerImplTest, ObserverNotified) {
  TestObserver observer(network_change_manager_client());
  ASSERT_EQ(mojom::ConnectionType::CONNECTION_UNKNOWN,
            network_change_manager_client()->GetConnectionType());
  ASSERT_EQ(mojom::ConnectionSubtype::SUBTYPE_UNKNOWN,
            network_change_manager_client()->GetConnectionSubtype());

  // Simulate a network change.
  net::NetworkChangeNotifier::NotifyObserversOfNetworkChangeForTests(
      net::NetworkChangeNotifier::CONNECTION_3G);
  observer.WaitForNotification();
  ASSERT_EQ(mojom::ConnectionType::CONNECTION_3G,
            network_change_manager_client()->GetConnectionType());
  ASSERT_EQ(mojom::ConnectionSubtype::SUBTYPE_UNKNOWN,
            network_change_manager_client()->GetConnectionSubtype());
}

}  // namespace content
