// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/bluetooth/bluetooth_power_controller.h"

#include "ash/public/cpp/ash_pref_names.h"
#include "ash/shell.h"
#include "ash/test/ash_test_base.h"
#include "ash/test/ash_test_helper.h"
#include "ash/test_shell_delegate.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#include "device/bluetooth/dbus/fake_bluetooth_adapter_client.h"

namespace ash {
namespace {

constexpr char kUser1Email[] = "user1@bluetooth";

}  // namespace

class BluetoothPowerControllerTest : public AshTestBase {
 public:
  BluetoothPowerControllerTest() {}
  ~BluetoothPowerControllerTest() override {}

  void SetUp() override {
    AshTestBase::SetUp();

    // Set Bluetooth discovery simulation delay to 0 so the test doesn't have to
    // wait or use timers.
    bluez::FakeBluetoothAdapterClient* adapter_client =
        static_cast<bluez::FakeBluetoothAdapterClient*>(
            bluez::BluezDBusManager::Get()->GetBluetoothAdapterClient());
    adapter_client->SetSimulationIntervalMs(0);

    active_user_prefs_.registry()->RegisterBooleanPref(
        prefs::kBluetoothAdapterEnabled, false);
    local_state_prefs_.registry()->RegisterBooleanPref(
        prefs::kSystemBluetoothAdapterEnabled, false);

    GetController()->active_user_pref_service_ = &active_user_prefs_;
    GetController()->local_state_pref_service_ = &local_state_prefs_;
    Shell::Get()
        ->bluetooth_power_controller()
        ->StartWatchingLocalStatePrefsChanges();
    Shell::Get()
        ->bluetooth_power_controller()
        ->StartWatchingActiveUserPrefsChanges();

    GetSessionControllerClient()->Reset();

    // Makes sure we get the callback from BluetoothAdapterFactory::GetAdapter
    // first before running the remaining test.
    RunAllPendingInMessageLoop();
  }

  void TearDown() override { AshTestBase::TearDown(); }

 protected:
  BluetoothPowerController* GetController() {
    return Shell::Get()->bluetooth_power_controller();
  }

  device::BluetoothAdapter* GetBluetoothAdapter() {
    return Shell::Get()->bluetooth_power_controller()->bluetooth_adapter_.get();
  }

  void ApplyBluetoothLocalStatePref() {
    Shell::Get()->bluetooth_power_controller()->ApplyBluetoothLocalStatePref();
  }

  void ApplyBluetoothPrimaryUserPref() {
    Shell::Get()->bluetooth_power_controller()->ApplyBluetoothPrimaryUserPref();
  }

  TestingPrefServiceSimple active_user_prefs_;
  TestingPrefServiceSimple local_state_prefs_;

 private:
  DISALLOW_COPY_AND_ASSIGN(BluetoothPowerControllerTest);
};

// Tests toggling Bluetooth setting on and off.
TEST_F(BluetoothPowerControllerTest, ToggleBluetoothEnabled) {
  // Toggles bluetooth on/off when there is no user session should affect
  // local state prefs.
  EXPECT_FALSE(
      local_state_prefs_.GetBoolean(prefs::kSystemBluetoothAdapterEnabled));
  GetController()->ToggleBluetoothEnabled();
  EXPECT_TRUE(
      local_state_prefs_.GetBoolean(prefs::kSystemBluetoothAdapterEnabled));
  GetController()->ToggleBluetoothEnabled();
  EXPECT_FALSE(
      local_state_prefs_.GetBoolean(prefs::kSystemBluetoothAdapterEnabled));

  // Toggles bluetooth on/off when there is user session should affect
  // puser prefs.
  GetSessionControllerClient()->AddUserSession(kUser1Email);
  EXPECT_FALSE(active_user_prefs_.GetBoolean(prefs::kBluetoothAdapterEnabled));
  GetController()->ToggleBluetoothEnabled();
  EXPECT_TRUE(active_user_prefs_.GetBoolean(prefs::kBluetoothAdapterEnabled));
  GetController()->ToggleBluetoothEnabled();
  EXPECT_FALSE(active_user_prefs_.GetBoolean(prefs::kBluetoothAdapterEnabled));
}

// Tests that BluetoothPowerController listens to local state pref changes
// and applies the changes to bluetooth device.
TEST_F(BluetoothPowerControllerTest, ListensPrefChangesLocalState) {
  // Makes sure we start with bluetooth power off.
  EXPECT_FALSE(GetBluetoothAdapter()->IsPowered());
  EXPECT_FALSE(
      local_state_prefs_.GetBoolean(prefs::kSystemBluetoothAdapterEnabled));

  // Power should be turned on when pref changes to enabled.
  local_state_prefs_.SetBoolean(prefs::kSystemBluetoothAdapterEnabled, true);
  EXPECT_TRUE(GetBluetoothAdapter()->IsPowered());

  // Power should be turned off when pref changes to disabled.
  local_state_prefs_.SetBoolean(prefs::kSystemBluetoothAdapterEnabled, false);
  EXPECT_FALSE(GetBluetoothAdapter()->IsPowered());
}

// Tests that BluetoothPowerController listens to active user pref changes
// and applies the changes to bluetooth device.
TEST_F(BluetoothPowerControllerTest, ListensPrefChangesActiveUser) {
  // Makes sure we start with bluetooth power off.
  EXPECT_FALSE(GetBluetoothAdapter()->IsPowered());
  EXPECT_FALSE(active_user_prefs_.GetBoolean(prefs::kBluetoothAdapterEnabled));

  // Power should be turned on when pref changes to enabled.
  active_user_prefs_.SetBoolean(prefs::kBluetoothAdapterEnabled, true);
  EXPECT_TRUE(GetBluetoothAdapter()->IsPowered());

  // Power should be turned off when pref changes to disabled.
  active_user_prefs_.SetBoolean(prefs::kBluetoothAdapterEnabled, false);
  EXPECT_FALSE(GetBluetoothAdapter()->IsPowered());
}

// Tests how BluetoothPowerController applies the local state pref when
// the pref hasn't been set before.
TEST_F(BluetoothPowerControllerTest, ApplyBluetoothLocalStatePrefDefault) {
  // Makes sure pref hasn't been set before.
  EXPECT_TRUE(
      local_state_prefs_.FindPreference(prefs::kSystemBluetoothAdapterEnabled)
          ->IsDefaultValue());
  // Start with bluetooth power on.
  GetBluetoothAdapter()->SetPowered(true, base::Bind(&base::DoNothing),
                                    base::Bind(&base::DoNothing));
  EXPECT_TRUE(GetBluetoothAdapter()->IsPowered());

  ApplyBluetoothLocalStatePref();

  // Pref should now contain the current bluetooth adapter state (on).
  EXPECT_FALSE(
      local_state_prefs_.FindPreference(prefs::kSystemBluetoothAdapterEnabled)
          ->IsDefaultValue());
  EXPECT_TRUE(
      local_state_prefs_.GetBoolean(prefs::kSystemBluetoothAdapterEnabled));
}

// Tests how BluetoothPowerController applies the local state pref when
// the pref has been set before.
TEST_F(BluetoothPowerControllerTest, ApplyBluetoothLocalStatePrefOn) {
  // Set the pref to true.
  local_state_prefs_.SetBoolean(prefs::kSystemBluetoothAdapterEnabled, true);
  EXPECT_FALSE(
      local_state_prefs_.FindPreference(prefs::kSystemBluetoothAdapterEnabled)
          ->IsDefaultValue());
  // Start with bluetooth power off.
  GetBluetoothAdapter()->SetPowered(false, base::Bind(&base::DoNothing),
                                    base::Bind(&base::DoNothing));
  EXPECT_FALSE(GetBluetoothAdapter()->IsPowered());

  ApplyBluetoothLocalStatePref();

  // Bluetooth power setting should be applied (on), and pref value unchanged.
  EXPECT_TRUE(
      local_state_prefs_.GetBoolean(prefs::kSystemBluetoothAdapterEnabled));
  EXPECT_TRUE(GetBluetoothAdapter()->IsPowered());
}

// Tests how BluetoothPowerController applies the user pref when
// the pref hasn't been set before.
TEST_F(BluetoothPowerControllerTest, ApplyBluetoothPrimaryUserPrefDefault) {
  GetSessionControllerClient()->AddUserSession(kUser1Email);

  // Makes sure pref hasn't been set before.
  EXPECT_TRUE(
      active_user_prefs_.FindPreference(prefs::kBluetoothAdapterEnabled)
          ->IsDefaultValue());
  // Start with bluetooth power off.
  GetBluetoothAdapter()->SetPowered(false, base::Bind(&base::DoNothing),
                                    base::Bind(&base::DoNothing));
  EXPECT_FALSE(GetBluetoothAdapter()->IsPowered());

  ApplyBluetoothPrimaryUserPref();

  // Pref should now contain the current bluetooth adapter state (off).
  EXPECT_FALSE(
      active_user_prefs_.FindPreference(prefs::kBluetoothAdapterEnabled)
          ->IsDefaultValue());
  EXPECT_FALSE(active_user_prefs_.GetBoolean(prefs::kBluetoothAdapterEnabled));
}

// Tests how BluetoothPowerController applies the user pref when
// the pref hasn't been set before, and it's a first-login user.
TEST_F(BluetoothPowerControllerTest, ApplyBluetoothPrimaryUserPrefDefaultNew) {
  GetSessionControllerClient()->AddUserSession(
      kUser1Email, user_manager::USER_TYPE_REGULAR, false, true);

  // Makes sure pref hasn't been set before.
  EXPECT_TRUE(
      active_user_prefs_.FindPreference(prefs::kBluetoothAdapterEnabled)
          ->IsDefaultValue());
  // Start with bluetooth power off.
  GetBluetoothAdapter()->SetPowered(false, base::Bind(&base::DoNothing),
                                    base::Bind(&base::DoNothing));
  EXPECT_FALSE(GetBluetoothAdapter()->IsPowered());

  ApplyBluetoothPrimaryUserPref();

  // Pref should be set to true for first-login users, and this will also
  // trigger the bluetooth power on.
  EXPECT_FALSE(
      active_user_prefs_.FindPreference(prefs::kBluetoothAdapterEnabled)
          ->IsDefaultValue());
  EXPECT_TRUE(active_user_prefs_.GetBoolean(prefs::kBluetoothAdapterEnabled));
  EXPECT_TRUE(GetBluetoothAdapter()->IsPowered());
}

// Tests how BluetoothPowerController applies the user pref when
// the pref hasn't been set before, but not a regular user (.e.g kiosk).
TEST_F(BluetoothPowerControllerTest, ApplyBluetoothKioskUserPrefDefault) {
  GetSessionControllerClient()->AddUserSession(
      kUser1Email, user_manager::USER_TYPE_KIOSK_APP);

  // Makes sure pref hasn't been set before.
  EXPECT_TRUE(
      active_user_prefs_.FindPreference(prefs::kBluetoothAdapterEnabled)
          ->IsDefaultValue());
  // Start with bluetooth power off.
  GetBluetoothAdapter()->SetPowered(false, base::Bind(&base::DoNothing),
                                    base::Bind(&base::DoNothing));
  EXPECT_FALSE(GetBluetoothAdapter()->IsPowered());

  ApplyBluetoothPrimaryUserPref();

  // For non-regular user, do not apply the bluetooth setting and no need
  // to set the pref.
  EXPECT_TRUE(
      active_user_prefs_.FindPreference(prefs::kBluetoothAdapterEnabled)
          ->IsDefaultValue());
  EXPECT_FALSE(active_user_prefs_.GetBoolean(prefs::kBluetoothAdapterEnabled));
  EXPECT_FALSE(GetBluetoothAdapter()->IsPowered());
}

// Tests how BluetoothPowerController applies the user pref when
// the pref has been set before.
TEST_F(BluetoothPowerControllerTest, ApplyBluetoothPrimaryUserPrefOn) {
  GetSessionControllerClient()->AddUserSession(kUser1Email);

  // Set the pref to true.
  active_user_prefs_.SetBoolean(prefs::kBluetoothAdapterEnabled, true);
  EXPECT_FALSE(
      active_user_prefs_.FindPreference(prefs::kBluetoothAdapterEnabled)
          ->IsDefaultValue());
  // Start with bluetooth power off.
  GetBluetoothAdapter()->SetPowered(false, base::Bind(&base::DoNothing),
                                    base::Bind(&base::DoNothing));
  EXPECT_FALSE(GetBluetoothAdapter()->IsPowered());

  ApplyBluetoothPrimaryUserPref();

  // Pref should be applied to trigger the bluetooth power on, and the pref
  // value should be unchanged..
  EXPECT_TRUE(active_user_prefs_.GetBoolean(prefs::kBluetoothAdapterEnabled));
  EXPECT_TRUE(GetBluetoothAdapter()->IsPowered());
}

}  // namespace ash
