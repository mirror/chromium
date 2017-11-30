// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_ble_connection.h"

#include "base/bind.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "device/bluetooth/public/interfaces/test/fake_bluetooth.mojom.h"
#include "device/bluetooth/test/bluetooth_test.h"
#include "device/bluetooth/test/fake_bluetooth.h"
#include "device/u2f/u2f_ble_uuids.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_ANDROID)
#include "device/bluetooth/test/bluetooth_test_android.h"
#elif defined(OS_MACOSX)
#include "device/bluetooth/test/bluetooth_test_mac.h"
#elif defined(OS_WIN)
#include "device/bluetooth/test/bluetooth_test_win.h"
#elif defined(OS_CHROMEOS) || defined(OS_LINUX)
#include "device/bluetooth/test/bluetooth_test_bluez.h"
#endif

namespace device {

class U2fBleConnectionTest : public BluetoothTest {
 public:
  U2fBleConnectionTest() {
    fake_bluetooth_.SimulateCentral(
        bluetooth::mojom::CentralState::POWERED_ON,
        base::BindOnce(&U2fBleConnectionTest::OnSimulateCentral,
                       base::Unretained(this)));
    set_up_loop_.Run();
  }

  void SetUpU2fDevice(std::string device_address) {
    fake_central_ptr_->SimulatePreconnectedPeripheral(
        device_address, kTestDeviceNameU2f, {},
        base::BindOnce(&U2fBleConnectionTest::OnPeripheral,
                       base::Unretained(this), device_address));
    base::RunLoop().RunUntilIdle();
  }

 private:
  void OnSimulateCentral(bluetooth::mojom::FakeCentralPtr fake_central_ptr) {
    fake_central_ptr_ = std::move(fake_central_ptr);
    set_up_loop_.Quit();
  }

  void OnPeripheral(const std::string& device_address) {
    auto do_nothing = [](bool) {};
    fake_central_ptr_->SetNextGATTConnectionResponse(
        device_address, bluetooth::mojom::kGATTSuccess,
        base::BindOnce(do_nothing));

    fake_central_ptr_->SetNextGATTDiscoveryResponse(
        device_address, bluetooth::mojom::kGATTSuccess,
        base::BindOnce(do_nothing));

    fake_central_ptr_->AddFakeService(
        device_address, BluetoothUUID(U2F_SERVICE_UUID),
        base::BindOnce(&U2fBleConnectionTest::OnAddFakeService,
                       base::Unretained(this), device_address));
  }

  void OnAddFakeService(const std::string& device_address,
                        const base::Optional<std::string>& service_id) {
    DCHECK(service_id);
    bluetooth::mojom::CharacteristicPropertiesPtr write_prop(base::in_place);
    write_prop->write = true;

    bluetooth::mojom::CharacteristicPropertiesPtr read_prop(base::in_place);
    read_prop->read = true;

    bluetooth::mojom::CharacteristicPropertiesPtr rw_prop(base::in_place);
    rw_prop->read = true;
    rw_prop->write = true;

    bluetooth::mojom::CharacteristicPropertiesPtr notify_prop(base::in_place);
    notify_prop->notify = true;

    auto do_nothing = [](const base::Optional<std::string>&) {};
    fake_central_ptr_->AddFakeCharacteristic(
        BluetoothUUID(U2F_CONTROL_POINT_UUID), write_prop->Clone(), *service_id,
        device_address, base::BindOnce(do_nothing));

    fake_central_ptr_->AddFakeCharacteristic(
        BluetoothUUID(U2F_STATUS_UUID), notify_prop->Clone(), *service_id,
        device_address,
        base::BindOnce(&U2fBleConnectionTest::OnAddFakeStatusCharacteristic,
                       base::Unretained(this), device_address, *service_id));

    fake_central_ptr_->AddFakeCharacteristic(
        BluetoothUUID(U2F_CONTROL_POINT_LENGTH_UUID), read_prop->Clone(),
        *service_id, device_address, base::BindOnce(do_nothing));

    fake_central_ptr_->AddFakeCharacteristic(
        BluetoothUUID(U2F_SERVICE_REVISION_UUID), read_prop->Clone(),
        *service_id, device_address, base::BindOnce(do_nothing));

    fake_central_ptr_->AddFakeCharacteristic(
        BluetoothUUID(U2F_SERVICE_REVISION_BITFIELD_UUID), rw_prop->Clone(),
        *service_id, device_address, base::BindOnce(do_nothing));
  }

  void OnAddFakeStatusCharacteristic(
      const std::string& device_address,
      const std::string& service_id,
      const base::Optional<std::string>& characteristic_id) {
    DCHECK(characteristic_id);
    auto do_nothing_opt_str = [](const base::Optional<std::string>&) {};
    fake_central_ptr_->AddFakeDescriptor(
        BluetoothRemoteGattDescriptor::ClientCharacteristicConfigurationUuid(),
        *characteristic_id, service_id, device_address,
        base::BindOnce(do_nothing_opt_str));

    auto do_nothing_bool = [](bool) {};
    fake_central_ptr_->SetNextSubscribeToNotificationsResponse(
        bluetooth::mojom::kGATTSuccess, *characteristic_id, service_id,
        device_address, base::BindOnce(do_nothing_bool));
  }

  bluetooth::FakeBluetooth fake_bluetooth_;
  bluetooth::mojom::FakeCentralPtr fake_central_ptr_;

  base::RunLoop set_up_loop_;
};

class TestConnectCallback {
 public:
  void OnConnect(bool success) {
    success_ = success;
    run_loop_.Quit();
  }

  bool WaitForCallback() {
    run_loop_.Run();
    return success_;
  }

  U2fBleConnection::ConnectCallback GetCallback() {
    return base::BindOnce(&TestConnectCallback::OnConnect,
                          base::Unretained(this));
  }

 private:
  bool success_ = false;
  U2fBleConnection::ConnectCallback callback_;
  base::RunLoop run_loop_;
};

TEST_F(U2fBleConnectionTest, Address) {
  auto read_do_nothing = [](bool) {};
  auto write_do_nothing = [](std::vector<uint8_t>) {};
  U2fBleConnection connection(kTestDeviceAddress1,
                              base::BindOnce(read_do_nothing),
                              base::BindRepeating(write_do_nothing));

  EXPECT_EQ(kTestDeviceAddress1, connection.address());
}

TEST_F(U2fBleConnectionTest, Connection) {
  TestConnectCallback connect_callback;
  SetUpU2fDevice(kTestDeviceAddress1);

  auto do_nothing = [](std::vector<uint8_t>) {};
  U2fBleConnection connection(kTestDeviceAddress1,
                              connect_callback.GetCallback(),
                              base::BindRepeating(do_nothing));

  EXPECT_TRUE(connect_callback.WaitForCallback());
}

}  // namespace device
