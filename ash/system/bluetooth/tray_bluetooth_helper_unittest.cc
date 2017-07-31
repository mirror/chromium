// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ash/system/bluetooth/tray_bluetooth_helper.h"

#include <vector>

#include "ash/system/tray/system_tray_delegate.h"
#include "ash/test/ash_test_base.h"
#include "device/bluetooth/dbus/bluez_dbus_manager.h"
#include "device/bluetooth/dbus/fake_bluetooth_adapter_client.h"

using bluez::BluezDBusManager;
using bluez::FakeBluetoothAdapterClient;

namespace ash {
namespace {

class TestBluetoothAdapterClient : public ash::mojom::BluetoothAdapterClient {
 public:
  explicit TestBluetoothAdapterClient(
      ash::mojom::BluetoothAdapterClientRequest request)
      : binding_(this, std::move(request)) {}

  void OnAdapterPowerSet(bool enabled) override { adapter_state_ = enabled; }

  bool GetAdapterState() { return adapter_state_; }

 private:
  mojo::Binding<ash::mojom::BluetoothAdapterClient> binding_;
  bool adapter_state_ = false;
};

using TrayBluetoothHelperTest = AshTestBase;

// Tests basic functionality like turning Bluetooth on and off.
TEST_F(TrayBluetoothHelperTest, Basics) {
  // Set Bluetooth discovery simulation delay to 0 so the test doesn't have to
  // wait or use timers.
  FakeBluetoothAdapterClient* adapter_client =
      static_cast<FakeBluetoothAdapterClient*>(
          BluezDBusManager::Get()->GetBluetoothAdapterClient());
  adapter_client->SetSimulationIntervalMs(0);

  TrayBluetoothHelper helper;
  ash::mojom::BluetoothAdapterClientPtr client_ptr;
  TestBluetoothAdapterClient client(mojo::MakeRequest(&client_ptr));
  helper.SetClient(std::move(client_ptr));

  helper.Initialize();
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(helper.GetBluetoothAvailable());
  EXPECT_FALSE(helper.GetBluetoothEnabled());
  EXPECT_FALSE(helper.HasBluetoothDiscoverySession());
  EXPECT_FALSE(client.GetAdapterState());

  BluetoothDeviceList devices = helper.GetAvailableBluetoothDevices();
  // The devices are fake in tests, so don't assume any particular number.
  EXPECT_FALSE(devices.empty());

  // Turn Bluetooth on.
  helper.ToggleBluetoothEnabled();
  RunAllPendingInMessageLoop();
  EXPECT_TRUE(helper.GetBluetoothEnabled());
  EXPECT_TRUE(client.GetAdapterState());

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
  EXPECT_FALSE(client.GetAdapterState());
}

}  // namespace
}  // namespace ash
