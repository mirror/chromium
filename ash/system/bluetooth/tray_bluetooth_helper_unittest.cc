// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/bluetooth/tray_bluetooth_helper.h"

#include <vector>

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/system/tray/system_tray_delegate.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ash/test_shell_delegate.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/user_manager/fake_user_manager.h"
#include "components/user_manager/user_manager.h"
#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#include "device/bluetooth/dbus/fake_bluetooth_adapter_client.h"

using bluez::BluezDBusManager;
using bluez::FakeBluetoothAdapterClient;

namespace ash {
namespace {

class TrayBluetoothHelperTest : public AshTestBase {
 public:
  void SetUp() override {
    AshTestBase::SetUp();
    fake_user_manager_.Initialize();
    fake_user_manager_.AddUser(AccountId::FromUserEmail("test@test.com"));
    local_state_pref_service_.registry()->RegisterBooleanPref(
        prefs::kBluetoothAdapterEnabled, false);
    active_user_pref_service_.registry()->RegisterBooleanPref(
        prefs::kBluetoothAdapterEnabled, false);
    ash_test_helper()->test_shell_delegate()->set_local_state_pref_service(
        &local_state_pref_service_);
    ash_test_helper()->test_shell_delegate()->set_active_user_pref_service(
        &active_user_pref_service_);

    // Set Bluetooth discovery simulation delay to 0 so the test doesn't have to
    // wait or use timers.
    FakeBluetoothAdapterClient* adapter_client =
        static_cast<FakeBluetoothAdapterClient*>(
            BluezDBusManager::Get()->GetBluetoothAdapterClient());
    adapter_client->SetSimulationIntervalMs(0);
  }

  void TearDown() override {
    AshTestBase::TearDown();
    fake_user_manager_.Destroy();
  }

 protected:
  TestingPrefServiceSimple local_state_pref_service_;
  TestingPrefServiceSimple active_user_pref_service_;
  user_manager::FakeUserManager fake_user_manager_;
};

// Tests basic functionality like turning Bluetooth on and off.
TEST_F(TrayBluetoothHelperTest, Basics) {
  TrayBluetoothHelper helper;
  helper.Initialize();
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(helper.GetBluetoothAvailable());
  EXPECT_FALSE(helper.GetBluetoothEnabled());
  EXPECT_FALSE(helper.HasBluetoothDiscoverySession());

  BluetoothDeviceList devices = helper.GetAvailableBluetoothDevices();
  // The devices are fake in tests, so don't assume any particular number.
  EXPECT_FALSE(devices.empty());

  // Turn Bluetooth on.
  helper.ToggleBluetoothEnabled();
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(helper.GetBluetoothEnabled());

  helper.StartBluetoothDiscovering();
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(helper.HasBluetoothDiscoverySession());

  helper.StopBluetoothDiscovering();
  RunAllPendingInMessageLoop();
  EXPECT_FALSE(helper.HasBluetoothDiscoverySession());

  // Turn Bluetooth off.
  helper.ToggleBluetoothEnabled();
  RunAllPendingInMessageLoop();
  EXPECT_FALSE(helper.GetBluetoothEnabled());
}

TEST_F(TrayBluetoothHelperTest, LocalStatePrefSaved) {
  TrayBluetoothHelper helper;
  helper.Initialize();
  RunAllPendingInMessageLoop();

  EXPECT_FALSE(helper.GetBluetoothEnabled());
  EXPECT_FALSE(
      local_state_pref_service_.GetBoolean(prefs::kBluetoothAdapterEnabled));

  // Turn bluetooth on.
  helper.ToggleBluetoothEnabled();
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(helper.GetBluetoothEnabled());
  EXPECT_TRUE(
      local_state_pref_service_.GetBoolean(prefs::kBluetoothAdapterEnabled));

  // Turn bluetooth off.
  helper.ToggleBluetoothEnabled();
  RunAllPendingInMessageLoop();
  EXPECT_FALSE(helper.GetBluetoothEnabled());
  EXPECT_FALSE(
      local_state_pref_service_.GetBoolean(prefs::kBluetoothAdapterEnabled));
}

TEST_F(TrayBluetoothHelperTest, ActiveUserPrefSaved) {
  TrayBluetoothHelper helper;
  helper.Initialize();
  RunAllPendingInMessageLoop();

  // Add fake user login so user pref will be used instead of local state.
  std::vector<user_manager::User*> users = fake_user_manager_.GetUsers();
  fake_user_manager_.UserLoggedIn(users[0]->GetAccountId(),
                                  users[0]->username_hash(), false);

  EXPECT_FALSE(helper.GetBluetoothEnabled());
  EXPECT_FALSE(
      active_user_pref_service_.GetBoolean(prefs::kBluetoothAdapterEnabled));

  // Turn bluetooth on.
  helper.ToggleBluetoothEnabled();
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(helper.GetBluetoothEnabled());
  EXPECT_TRUE(
      active_user_pref_service_.GetBoolean(prefs::kBluetoothAdapterEnabled));

  // Turn bluetooth off.
  helper.ToggleBluetoothEnabled();
  RunAllPendingInMessageLoop();
  EXPECT_FALSE(helper.GetBluetoothEnabled());
  EXPECT_FALSE(
      active_user_pref_service_.GetBoolean(prefs::kBluetoothAdapterEnabled));
}

}  // namespace
}  // namespace ash
