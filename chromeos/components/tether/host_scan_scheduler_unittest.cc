// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/components/tether/host_scan_scheduler.h"

#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "chromeos/components/tether/host_scanner.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/dbus/power_manager_client.h"
#include "chromeos/login/login_state.h"
#include "chromeos/network/network_handler.h"
#include "chromeos/network/network_state.h"
#include "chromeos/network/network_state_handler.h"
#include "chromeos/network/network_state_test.h"
#include "chromeos/network/network_type_pattern.h"
#include "components/cryptauth/cryptauth_device_manager.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace chromeos {

namespace tether {

namespace {

class FakeHostScanner : public HostScanner {
 public:
  FakeHostScanner()
      : HostScanner(nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr,
                    nullptr),
        num_scans_started_(0) {}
  ~FakeHostScanner() override {}

  void StartScan() override {
    is_scan_active_ = true;
    num_scans_started_++;
  }

  void StopScan() {
    is_scan_active_ = false;
    NotifyScanFinished();
  }

  bool IsScanActive() override { return is_scan_active_; }

  int num_scans_started() { return num_scans_started_; }

 private:
  bool is_scan_active_ = false;
  int num_scans_started_ = 0;
};

const char kEthernetServiceGuid[] = "ethernetServiceGuid";
const char kWifiServiceGuid[] = "wifiServiceGuid";
const char kTetherGuid[] = "tetherGuid";

std::string CreateConfigurationJsonString(const std::string& guid,
                                          const std::string& type) {
  std::stringstream ss;
  ss << "{"
     << "  \"GUID\": \"" << guid << "\","
     << "  \"Type\": \"" << type << "\","
     << "  \"State\": \"" << shill::kStateReady << "\""
     << "}";
  return ss.str();
}

}  // namespace

class HostScanSchedulerTest : public NetworkStateTest {
 protected:
  void SetUp() override {
    DBusThreadManager::Initialize();
    NetworkStateTest::SetUp();

    ethernet_service_path_ = ConfigureService(CreateConfigurationJsonString(
        kEthernetServiceGuid, shill::kTypeEthernet));
    test_manager_client()->SetManagerProperty(
        shill::kDefaultServiceProperty, base::Value(ethernet_service_path_));

    network_state_handler()->SetTetherTechnologyState(
        NetworkStateHandler::TECHNOLOGY_ENABLED);

    fake_host_scanner_ = base::MakeUnique<FakeHostScanner>();

    host_scan_scheduler_ = base::WrapUnique(new HostScanScheduler(
        network_state_handler(), fake_host_scanner_.get()));
  }

  void TearDown() override {
    host_scan_scheduler_.reset();

    ShutdownNetworkState();
    NetworkStateTest::TearDown();
    DBusThreadManager::Shutdown();
  }

  void SetDefaultNetworkDisconnected() {
    SetServiceProperty(ethernet_service_path_,
                       std::string(shill::kStateProperty),
                       base::Value(shill::kStateIdle));
  }

  void SetDefaultNetworkConnecting() {
    SetServiceProperty(ethernet_service_path_,
                       std::string(shill::kStateProperty),
                       base::Value(shill::kStateAssociation));
  }

  void SetDefaultNetworkConnected() {
    SetServiceProperty(ethernet_service_path_,
                       std::string(shill::kStateProperty),
                       base::Value(shill::kStateReady));
  }

  void AddTetherNetworkState() {
    network_state_handler()->AddTetherNetworkState(
        kTetherGuid, "name", "carrier", 100 /* battery_percentage */,
        100 /* signal strength */, false /* has_connected_to_host */);
    ConfigureService(
        CreateConfigurationJsonString(kWifiServiceGuid, shill::kTypeWifi));
    network_state_handler()->AssociateTetherNetworkStateWithWifiNetwork(
        kTetherGuid, kWifiServiceGuid);
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  std::string ethernet_service_path_;

  std::unique_ptr<FakeHostScanner> fake_host_scanner_;

  std::unique_ptr<HostScanScheduler> host_scan_scheduler_;
};

TEST_F(HostScanSchedulerTest, ScheduleScan) {
  EXPECT_EQ(0, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  host_scan_scheduler_->ScheduleScan();

  EXPECT_EQ(1, fake_host_scanner_->num_scans_started());
  EXPECT_TRUE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));
}

TEST_F(HostScanSchedulerTest, ScanRequested) {
  EXPECT_EQ(0, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  // Begin scanning.
  host_scan_scheduler_->ScanRequested();
  EXPECT_EQ(1, fake_host_scanner_->num_scans_started());
  EXPECT_TRUE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  // Should not begin a new scan while a scan is active.
  host_scan_scheduler_->ScanRequested();
  EXPECT_EQ(1, fake_host_scanner_->num_scans_started());
  EXPECT_TRUE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  fake_host_scanner_->StopScan();
  EXPECT_EQ(1, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  // A new scan should be allowed once a scan is not active.
  host_scan_scheduler_->ScanRequested();
  EXPECT_EQ(2, fake_host_scanner_->num_scans_started());
  EXPECT_TRUE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));
}

TEST_F(HostScanSchedulerTest, DefaultNetworkChanged) {
  EXPECT_EQ(0, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  SetDefaultNetworkConnecting();
  EXPECT_EQ(0, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  SetDefaultNetworkConnected();
  EXPECT_EQ(0, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  SetDefaultNetworkDisconnected();
  EXPECT_EQ(1, fake_host_scanner_->num_scans_started());
  EXPECT_TRUE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  fake_host_scanner_->StopScan();
  test_manager_client()->SetManagerProperty(
      shill::kDefaultServiceProperty, base::Value(ethernet_service_path_));

  // When Tether is present but disconnected, a scan should start when the
  // default network is disconnected.
  AddTetherNetworkState();

  EXPECT_EQ(1, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  SetDefaultNetworkConnecting();
  EXPECT_EQ(1, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  SetDefaultNetworkConnected();
  EXPECT_EQ(1, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  SetDefaultNetworkDisconnected();
  EXPECT_EQ(2, fake_host_scanner_->num_scans_started());
  EXPECT_TRUE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  fake_host_scanner_->StopScan();
  test_manager_client()->SetManagerProperty(
      shill::kDefaultServiceProperty, base::Value(ethernet_service_path_));

  // When Tether is present and connecting, no scan should start.
  network_state_handler()->SetTetherNetworkStateConnecting(kTetherGuid);

  EXPECT_EQ(2, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  SetDefaultNetworkConnecting();
  EXPECT_EQ(2, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  SetDefaultNetworkConnected();
  EXPECT_EQ(2, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  SetDefaultNetworkDisconnected();
  EXPECT_EQ(2, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  fake_host_scanner_->StopScan();
  test_manager_client()->SetManagerProperty(
      shill::kDefaultServiceProperty, base::Value(ethernet_service_path_));
  base::RunLoop().RunUntilIdle();

  // When Tether is present and connected, no scan should start.
  network_state_handler()->SetTetherNetworkStateConnected(kTetherGuid);

  EXPECT_EQ(2, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  SetDefaultNetworkConnecting();
  EXPECT_EQ(2, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  SetDefaultNetworkConnected();
  EXPECT_EQ(2, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));

  SetDefaultNetworkDisconnected();
  EXPECT_EQ(2, fake_host_scanner_->num_scans_started());
  EXPECT_FALSE(
      network_state_handler()->GetScanningByType(NetworkTypePattern::Tether()));
}

}  // namespace tether

}  // namespace chromeos
