// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/network_connection_handler_tether_delegate.h"

#include <memory>

#include "chromeos/components/tether/fake_active_host.h"
#include "chromeos/components/tether/fake_tether_connector.h"
#include "chromeos/components/tether/fake_tether_disconnector.h"
#include "chromeos/components/tether/tether_disconnector.h"
#include "chromeos/network/network_connection_handler.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace tether {

namespace {

class TestNetworkConnectionHandler : public NetworkConnectionHandler {
 public:
  TestNetworkConnectionHandler() : NetworkConnectionHandler() {}
  ~TestNetworkConnectionHandler() override {}

  void CallTetherConnect(const std::string& tether_network_guid,
                         const base::Closure& success_callback,
                         const network_handler::ErrorCallback& error_callback) {
    InitiateTetherNetworkConnection(tether_network_guid, success_callback,
                                    error_callback);
  }

  void CallTetherDisconnect(
      const std::string& tether_network_guid,
      const base::Closure& success_callback,
      const network_handler::ErrorCallback& error_callback) {
    InitiateTetherNetworkDisconnection(tether_network_guid, success_callback,
                                       error_callback);
  }

  // NetworkConnectionHandler:
  void ConnectToNetwork(const std::string& service_path,
                        const base::Closure& success_callback,
                        const network_handler::ErrorCallback& error_callback,
                        bool check_error_state) override {}

  void DisconnectNetwork(
      const std::string& service_path,
      const base::Closure& success_callback,
      const network_handler::ErrorCallback& error_callback) override {}

  bool HasConnectingNetwork(const std::string& service_path) override {
    return false;
  }

  bool HasPendingConnectRequest() override { return false; }

  void Init(NetworkStateHandler* network_state_handler,
            NetworkConfigurationHandler* network_configuration_handler,
            ManagedNetworkConfigurationHandler*
                managed_network_configuration_handler) override {}
};

}  // namespace

class NetworkConnectionHandlerTetherDelegateTest : public testing::Test {
 protected:
  NetworkConnectionHandlerTetherDelegateTest() {}

  void SetUp() override {
    test_network_connection_handler_ =
        base::WrapUnique(new TestNetworkConnectionHandler());

    fake_active_host_ = base::MakeUnique<FakeActiveHost>();
    fake_tether_connector_ = base::MakeUnique<FakeTetherConnector>();
    fake_tether_disconnector_ = base::MakeUnique<FakeTetherDisconnector>();

    delegate_ = base::MakeUnique<NetworkConnectionHandlerTetherDelegate>(
        test_network_connection_handler_.get(), fake_active_host_.get(),
        fake_tether_connector_.get(), fake_tether_disconnector_.get());
  }

  std::unique_ptr<TestNetworkConnectionHandler>
      test_network_connection_handler_;
  std::unique_ptr<FakeActiveHost> fake_active_host_;
  std::unique_ptr<FakeTetherConnector> fake_tether_connector_;
  std::unique_ptr<FakeTetherDisconnector> fake_tether_disconnector_;

  std::unique_ptr<NetworkConnectionHandlerTetherDelegate> delegate_;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkConnectionHandlerTetherDelegateTest);
};

TEST_F(NetworkConnectionHandlerTetherDelegateTest,
       TestConnect_NotAlreadyConnected) {
  test_network_connection_handler_->CallTetherConnect(
      "tetherNetworkGuid", base::Closure(), network_handler::ErrorCallback());
  EXPECT_EQ("tetherNetworkGuid",
            fake_tether_connector_->last_connected_tether_network_guid());
}

TEST_F(NetworkConnectionHandlerTetherDelegateTest,
       TestConnect_AlreadyConnected) {
  fake_active_host_->SetActiveHostConnected("activeHostId", "tetherNetworkGuid",
                                            "wifiNetworkGuid");

  test_network_connection_handler_->CallTetherConnect(
      "newTetherGuid", base::Closure(), network_handler::ErrorCallback());
  EXPECT_EQ("tetherNetworkGuid",
            fake_tether_disconnector_->last_disconnected_tether_network_guid());
  EXPECT_EQ("newTetherGuid",
            fake_tether_connector_->last_connected_tether_network_guid());
}

TEST_F(NetworkConnectionHandlerTetherDelegateTest, TestDisconnect) {
  test_network_connection_handler_->CallTetherDisconnect(
      "tetherNetworkGuid", base::Closure(), network_handler::ErrorCallback());
  EXPECT_EQ("tetherNetworkGuid",
            fake_tether_disconnector_->last_disconnected_tether_network_guid());
}

}  // namespace tether

}  // namespace chromeos
