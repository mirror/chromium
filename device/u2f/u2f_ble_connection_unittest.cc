// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/u2f/u2f_ble_connection.h"

#include <utility>

#include "base/bind.h"
#include "base/run_loop.h"
#include "build/build_config.h"
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

using ::testing::ElementsAre;
using ::testing::IsEmpty;

// TODO(crbug.com/763303): Add test for notifying ReadCallback once
// FakePeriphial supports it.
namespace device {

namespace {

uint16_t GetGattCode(bool success) {
  return success ? bluetooth::mojom::kGATTSuccess
                 : bluetooth::mojom::kGATTInvalidHandle;
}

std::vector<uint8_t> MakeVector(base::StringPiece str) {
  return std::vector<uint8_t>(str.begin(), str.end());
}

std::vector<uint8_t> MakeVector(uint8_t value) {
  return {value};
}

std::vector<uint8_t> MakeVector(uint8_t val0, uint8_t val1) {
  return {val0, val1};
}

class TestConnectCallback {
 public:
  void OnConnect(bool success) { success_ = success; }

  bool WaitForCallback() {
    base::RunLoop().RunUntilIdle();
    return success_;
  }

  U2fBleConnection::ConnectCallback GetCallback() {
    return base::BindRepeating(&TestConnectCallback::OnConnect,
                               base::Unretained(this));
  }

 private:
  bool success_ = false;
};

class TestReadControlPointLengthCallback {
 public:
  void OnReadControlPointLength(base::Optional<uint16_t> value) {
    value_ = std::move(value);
  }

  const base::Optional<uint16_t>& WaitForCallback() {
    base::RunLoop().RunUntilIdle();
    return value_;
  }

  U2fBleConnection::ControlPointLengthCallback GetCallback() {
    return base::BindOnce(
        &TestReadControlPointLengthCallback::OnReadControlPointLength,
        base::Unretained(this));
  }

 private:
  base::Optional<uint16_t> value_;
};

class TestReadServiceRevisionsCallback {
 public:
  void OnReadServiceRevisions(
      std::set<U2fBleConnection::ServiceRevision> revisions) {
    revisions_ = std::move(revisions);
  }

  const std::set<U2fBleConnection::ServiceRevision>& WaitForCallback() {
    base::RunLoop().RunUntilIdle();
    return revisions_;
  }

  U2fBleConnection::ServiceRevisionsCallback GetCallback() {
    return base::BindOnce(
        &TestReadServiceRevisionsCallback::OnReadServiceRevisions,
        base::Unretained(this));
  }

 private:
  std::set<U2fBleConnection::ServiceRevision> revisions_;
};

class TestWriteCallback {
 public:
  void OnWrite(bool success) { success_ = success; }

  bool WaitForCallback() {
    base::RunLoop().RunUntilIdle();
    return success_;
  }

  U2fBleConnection::WriteCallback GetCallback() {
    return base::BindOnce(&TestWriteCallback::OnWrite, base::Unretained(this));
  }

 private:
  bool success_ = false;
};

}  // namespace

class U2fBleConnectionTest : public BluetoothTest {
 public:
  U2fBleConnectionTest() {
    fake_bluetooth_.SimulateCentral(
        bluetooth::mojom::CentralState::POWERED_ON,
        base::BindOnce(&U2fBleConnectionTest::OnSimulateCentral,
                       base::Unretained(this)));
    set_up_loop_.Run();
  }

  void ConnectU2fDevice(const std::string& device_address) {
    fake_central_ptr_->SimulatePreconnectedPeripheral(
        device_address, kTestDeviceNameU2f, {},
        base::BindOnce(&U2fBleConnectionTest::OnPeripheral,
                       base::Unretained(this), device_address));
    base::RunLoop().RunUntilIdle();
  }

  void DisconnectDevice(const std::string& device_address) {
    auto do_nothing = [](bool) {};
    fake_central_ptr_->SimulateGATTDisconnection(device_address,
                                                 base::BindOnce(do_nothing));
    base::RunLoop().RunUntilIdle();
  }

  void SetNextReadControlPointLengthReponse(
      const std::string& device_address,
      bool success,
      const base::Optional<std::vector<uint8_t>>& value) {
    auto do_nothing = [](bool) {};
    fake_central_ptr_->SetNextReadCharacteristicResponse(
        GetGattCode(success), value, control_point_length_id_, u2f_service_id_,
        device_address, base::BindOnce(do_nothing));
    base::RunLoop().RunUntilIdle();
  }

  void SetNextReadServiceRevisionResponse(
      const std::string& device_address,
      bool success,
      const base::Optional<std::vector<uint8_t>>& value) {
    auto do_nothing = [](bool) {};
    fake_central_ptr_->SetNextReadCharacteristicResponse(
        GetGattCode(success), value, service_revision_id_, u2f_service_id_,
        device_address, base::BindOnce(do_nothing));
    base::RunLoop().RunUntilIdle();
  }

  void SetNextReadServiceRevisionBitfieldResponse(
      const std::string& device_address,
      bool success,
      const base::Optional<std::vector<uint8_t>>& value) {
    auto do_nothing = [](bool) {};
    fake_central_ptr_->SetNextReadCharacteristicResponse(
        GetGattCode(success), value, service_revision_bitfield_id_,
        u2f_service_id_, device_address, base::BindOnce(do_nothing));
    base::RunLoop().RunUntilIdle();
  }

  void SetNextControlPointWriteResponse(const std::string& device_address,
                                        bool success) {
    auto do_nothing = [](bool) {};
    fake_central_ptr_->SetNextWriteCharacteristicResponse(
        GetGattCode(success), control_point_id_, u2f_service_id_,
        device_address, base::BindOnce(do_nothing));
    base::RunLoop().RunUntilIdle();
  }

  void SetNextWriteServiceRevisionResponse(const std::string& device_address,
                                           bool success) {
    auto do_nothing = [](bool) {};
    fake_central_ptr_->SetNextWriteCharacteristicResponse(
        GetGattCode(success), service_revision_bitfield_id_, u2f_service_id_,
        device_address, base::BindOnce(do_nothing));
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
        device_address, GetGattCode(true), base::BindOnce(do_nothing));

    fake_central_ptr_->SetNextGATTDiscoveryResponse(
        device_address, GetGattCode(true), base::BindOnce(do_nothing));

    fake_central_ptr_->AddFakeService(
        device_address, BluetoothUUID(U2F_SERVICE_UUID),
        base::BindOnce(&U2fBleConnectionTest::OnAddFakeU2fService,
                       base::Unretained(this), device_address));
  }

  void OnAddFakeU2fService(const std::string& device_address,
                           const base::Optional<std::string>& service_id) {
    u2f_service_id_ = std::move(*service_id);

    bluetooth::mojom::CharacteristicPropertiesPtr write_prop(base::in_place);
    write_prop->write = true;

    bluetooth::mojom::CharacteristicPropertiesPtr read_prop(base::in_place);
    read_prop->read = true;

    bluetooth::mojom::CharacteristicPropertiesPtr rw_prop(base::in_place);
    rw_prop->read = true;
    rw_prop->write = true;

    bluetooth::mojom::CharacteristicPropertiesPtr notify_prop(base::in_place);
    notify_prop->notify = true;

    auto set_characteristic_id =
        [](std::string* characteristic_id,
           const base::Optional<std::string>& opt_characteristic_id) {
          *characteristic_id = *opt_characteristic_id;
        };

    fake_central_ptr_->AddFakeCharacteristic(
        BluetoothUUID(U2F_CONTROL_POINT_UUID), write_prop->Clone(),
        u2f_service_id_, device_address,
        base::BindOnce(set_characteristic_id, &control_point_id_));

    fake_central_ptr_->AddFakeCharacteristic(
        BluetoothUUID(U2F_STATUS_UUID), notify_prop->Clone(), u2f_service_id_,
        device_address,
        base::BindOnce(&U2fBleConnectionTest::OnAddFakeStatusCharacteristic,
                       base::Unretained(this), device_address));

    fake_central_ptr_->AddFakeCharacteristic(
        BluetoothUUID(U2F_CONTROL_POINT_LENGTH_UUID), read_prop->Clone(),
        u2f_service_id_, device_address,
        base::BindOnce(set_characteristic_id, &control_point_length_id_));

    fake_central_ptr_->AddFakeCharacteristic(
        BluetoothUUID(U2F_SERVICE_REVISION_UUID), read_prop->Clone(),
        u2f_service_id_, device_address,
        base::BindOnce(set_characteristic_id, &service_revision_id_));

    fake_central_ptr_->AddFakeCharacteristic(
        BluetoothUUID(U2F_SERVICE_REVISION_BITFIELD_UUID), rw_prop->Clone(),
        u2f_service_id_, device_address,
        base::BindOnce(set_characteristic_id, &service_revision_bitfield_id_));
  }

  void OnAddFakeStatusCharacteristic(
      const std::string& device_address,
      const base::Optional<std::string>& characteristic_id) {
    status_id_ = *characteristic_id;

    auto set_descriptor_id =
        [](std::string* descriptor_id,
           const base::Optional<std::string>& opt_descriptor_id) {
          *descriptor_id = *opt_descriptor_id;
        };

    fake_central_ptr_->AddFakeDescriptor(
        BluetoothRemoteGattDescriptor::ClientCharacteristicConfigurationUuid(),
        status_id_, u2f_service_id_, device_address,
        base::BindOnce(set_descriptor_id, &ccc_descriptor_id_));

    auto do_nothing_bool = [](bool) {};
    fake_central_ptr_->SetNextSubscribeToNotificationsResponse(
        GetGattCode(true), status_id_, u2f_service_id_, device_address,
        base::BindOnce(do_nothing_bool));
  }

  bluetooth::FakeBluetooth fake_bluetooth_;
  bluetooth::mojom::FakeCentralPtr fake_central_ptr_;

  std::string u2f_service_id_;
  std::string control_point_id_;
  std::string status_id_;
  std::string control_point_length_id_;
  std::string service_revision_id_;
  std::string service_revision_bitfield_id_;
  std::string ccc_descriptor_id_;

  base::RunLoop set_up_loop_;
};

TEST_F(U2fBleConnectionTest, Address) {
  const std::string device_address = kTestDeviceAddress1;
  auto connect_do_nothing = [](bool) {};
  auto read_do_nothing = [](std::vector<uint8_t>) {};

  U2fBleConnection connection(device_address,
                              base::BindRepeating(connect_do_nothing),
                              base::BindRepeating(read_do_nothing));
  EXPECT_EQ(device_address, connection.address());
}

TEST_F(U2fBleConnectionTest, DeviceNotPresent) {
  const std::string device_address = kTestDeviceAddress1;
  TestConnectCallback connect_callback;
  auto read_do_nothing = [](std::vector<uint8_t>) {};

  U2fBleConnection connection(device_address, connect_callback.GetCallback(),
                              base::BindRepeating(read_do_nothing));
  EXPECT_FALSE(connect_callback.WaitForCallback());
}

TEST_F(U2fBleConnectionTest, Connection) {
  const std::string device_address = kTestDeviceAddress1;
  TestConnectCallback connect_callback;
  ConnectU2fDevice(device_address);

  auto do_nothing = [](std::vector<uint8_t>) {};
  U2fBleConnection connection(device_address, connect_callback.GetCallback(),
                              base::BindRepeating(do_nothing));
  EXPECT_TRUE(connect_callback.WaitForCallback());
}

TEST_F(U2fBleConnectionTest, DeviceDisconnect) {
  const std::string device_address = kTestDeviceAddress1;
  TestConnectCallback connect_callback;

  ConnectU2fDevice(device_address);
  auto do_nothing = [](std::vector<uint8_t>) {};
  U2fBleConnection connection(device_address, connect_callback.GetCallback(),
                              base::BindRepeating(do_nothing));
  EXPECT_TRUE(connect_callback.WaitForCallback());

  DisconnectDevice(device_address);
  EXPECT_FALSE(connect_callback.WaitForCallback());
}

TEST_F(U2fBleConnectionTest, ReadControlPointLength) {
  const std::string device_address = kTestDeviceAddress1;
  TestConnectCallback connect_callback;
  ConnectU2fDevice(device_address);
  auto read_do_nothing = [](std::vector<uint8_t>) {};

  U2fBleConnection connection(device_address, connect_callback.GetCallback(),
                              base::BindRepeating(read_do_nothing));
  EXPECT_TRUE(connect_callback.WaitForCallback());

  std::vector<uint8_t> empty;
  std::vector<uint8_t> one_byte(1);
  std::vector<uint8_t> two_bytes({0xAB, 0xCD});
  std::vector<uint8_t> three_bytes(3);

  TestReadControlPointLengthCallback length_callback;
  SetNextReadControlPointLengthReponse(device_address, false, base::nullopt);
  connection.ReadControlPointLength(length_callback.GetCallback());
  EXPECT_FALSE(length_callback.WaitForCallback());

  SetNextReadControlPointLengthReponse(device_address, true, empty);
  connection.ReadControlPointLength(length_callback.GetCallback());
  EXPECT_FALSE(length_callback.WaitForCallback());

  SetNextReadControlPointLengthReponse(device_address, true, one_byte);
  connection.ReadControlPointLength(length_callback.GetCallback());
  EXPECT_FALSE(length_callback.WaitForCallback());

  SetNextReadControlPointLengthReponse(device_address, true, two_bytes);
  connection.ReadControlPointLength(length_callback.GetCallback());
  EXPECT_EQ(0xABCD, *length_callback.WaitForCallback());

  SetNextReadControlPointLengthReponse(device_address, true, three_bytes);
  connection.ReadControlPointLength(length_callback.GetCallback());
  EXPECT_FALSE(length_callback.WaitForCallback());

  // Reads should always fail on a disconnected device.
  DisconnectDevice(device_address);
  EXPECT_FALSE(connect_callback.WaitForCallback());
  connection.ReadControlPointLength(length_callback.GetCallback());
  EXPECT_FALSE(length_callback.WaitForCallback());
}

TEST_F(U2fBleConnectionTest, ReadServiceRevisions) {
  const std::string device_address = kTestDeviceAddress1;
  TestConnectCallback connect_callback;
  ConnectU2fDevice(device_address);
  auto read_do_nothing = [](std::vector<uint8_t>) {};

  U2fBleConnection connection(device_address, connect_callback.GetCallback(),
                              base::BindRepeating(read_do_nothing));
  EXPECT_TRUE(connect_callback.WaitForCallback());

  TestReadServiceRevisionsCallback revisions_callback;
  SetNextReadServiceRevisionResponse(device_address, false, base::nullopt);
  SetNextReadServiceRevisionBitfieldResponse(device_address, false, {});
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(), IsEmpty());

  SetNextReadServiceRevisionResponse(device_address, true, MakeVector("bogus"));
  SetNextReadServiceRevisionBitfieldResponse(device_address, false,
                                             base::nullopt);
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(), IsEmpty());

  SetNextReadServiceRevisionResponse(device_address, true, MakeVector("1.0"));
  SetNextReadServiceRevisionBitfieldResponse(device_address, false,
                                             base::nullopt);
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(),
              ElementsAre(U2fBleConnection::ServiceRevision::VERSION_1_0));

  SetNextReadServiceRevisionResponse(device_address, true, MakeVector("1.1"));
  SetNextReadServiceRevisionBitfieldResponse(device_address, false,
                                             base::nullopt);
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(),
              ElementsAre(U2fBleConnection::ServiceRevision::VERSION_1_1));

  SetNextReadServiceRevisionResponse(device_address, true, MakeVector("1.2"));
  SetNextReadServiceRevisionBitfieldResponse(device_address, false,
                                             base::nullopt);
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(),
              ElementsAre(U2fBleConnection::ServiceRevision::VERSION_1_2));

  // Version 1.3 currently does not exist, so this should be treated as an
  // error.
  SetNextReadServiceRevisionResponse(device_address, true, MakeVector("1.3"));
  SetNextReadServiceRevisionBitfieldResponse(device_address, false,
                                             base::nullopt);
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(), IsEmpty());

  SetNextReadServiceRevisionResponse(device_address, false, base::nullopt);
  SetNextReadServiceRevisionBitfieldResponse(device_address, true,
                                             MakeVector(0x00));
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(), IsEmpty());

  SetNextReadServiceRevisionResponse(device_address, false, base::nullopt);
  SetNextReadServiceRevisionBitfieldResponse(device_address, true,
                                             MakeVector(0x80));
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(),
              ElementsAre(U2fBleConnection::ServiceRevision::VERSION_1_1));

  SetNextReadServiceRevisionResponse(device_address, false, base::nullopt);
  SetNextReadServiceRevisionBitfieldResponse(device_address, true,
                                             MakeVector(0x40));
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(),
              ElementsAre(U2fBleConnection::ServiceRevision::VERSION_1_2));

  SetNextReadServiceRevisionResponse(device_address, false, base::nullopt);
  SetNextReadServiceRevisionBitfieldResponse(device_address, true,
                                             MakeVector(0xC0));
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(),
              ElementsAre(U2fBleConnection::ServiceRevision::VERSION_1_1,
                          U2fBleConnection::ServiceRevision::VERSION_1_2));

  // All bits except the first two should be ignored.
  SetNextReadServiceRevisionResponse(device_address, false, base::nullopt);
  SetNextReadServiceRevisionBitfieldResponse(device_address, true,
                                             MakeVector(0xFF));
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(),
              ElementsAre(U2fBleConnection::ServiceRevision::VERSION_1_1,
                          U2fBleConnection::ServiceRevision::VERSION_1_2));

  // All bytes except the first one should be ignored.
  SetNextReadServiceRevisionResponse(device_address, false, base::nullopt);
  SetNextReadServiceRevisionBitfieldResponse(device_address, true,
                                             MakeVector(0xC0, 0xFF));
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(),
              ElementsAre(U2fBleConnection::ServiceRevision::VERSION_1_1,
                          U2fBleConnection::ServiceRevision::VERSION_1_2));

  // The combination of a service revision string and bitfield should be
  // supported as well.
  SetNextReadServiceRevisionResponse(device_address, true, MakeVector("1.0"));
  SetNextReadServiceRevisionBitfieldResponse(device_address, true,
                                             MakeVector(0xC0));
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(),
              ElementsAre(U2fBleConnection::ServiceRevision::VERSION_1_0,
                          U2fBleConnection::ServiceRevision::VERSION_1_1,
                          U2fBleConnection::ServiceRevision::VERSION_1_2));

  // Reads should always fail on a disconnected device.
  DisconnectDevice(device_address);
  EXPECT_FALSE(connect_callback.WaitForCallback());
  connection.ReadServiceRevisions(revisions_callback.GetCallback());
  EXPECT_THAT(revisions_callback.WaitForCallback(), IsEmpty());
}

TEST_F(U2fBleConnectionTest, WriteControlPoint) {
  const std::string device_address = kTestDeviceAddress1;
  TestConnectCallback connect_callback;
  ConnectU2fDevice(device_address);
  auto read_do_nothing = [](std::vector<uint8_t>) {};

  U2fBleConnection connection(device_address, connect_callback.GetCallback(),
                              base::BindRepeating(read_do_nothing));
  EXPECT_TRUE(connect_callback.WaitForCallback());

  TestWriteCallback write_callback;
  SetNextControlPointWriteResponse(device_address, false);
  connection.WriteControlPoint({}, write_callback.GetCallback());
  EXPECT_FALSE(write_callback.WaitForCallback());

  SetNextControlPointWriteResponse(device_address, true);
  connection.WriteControlPoint({}, write_callback.GetCallback());
  EXPECT_TRUE(write_callback.WaitForCallback());

  // Write should always fail on a disconnected device.
  DisconnectDevice(device_address);
  EXPECT_FALSE(connect_callback.WaitForCallback());
  connection.WriteControlPoint({}, write_callback.GetCallback());
  EXPECT_FALSE(write_callback.WaitForCallback());
}

TEST_F(U2fBleConnectionTest, WriteServiceRevision) {
  const std::string device_address = kTestDeviceAddress1;
  TestConnectCallback connect_callback;
  ConnectU2fDevice(device_address);
  auto read_do_nothing = [](std::vector<uint8_t>) {};

  U2fBleConnection connection(device_address, connect_callback.GetCallback(),
                              base::BindRepeating(read_do_nothing));
  EXPECT_TRUE(connect_callback.WaitForCallback());

  // Expect that errors are properly propagated.
  TestWriteCallback write_callback;
  SetNextWriteServiceRevisionResponse(device_address, false);
  connection.WriteServiceRevision(
      U2fBleConnection::ServiceRevision::VERSION_1_1,
      write_callback.GetCallback());
  EXPECT_FALSE(write_callback.WaitForCallback());

  // Expect a successful write of version 1.1.
  SetNextWriteServiceRevisionResponse(device_address, true);
  connection.WriteServiceRevision(
      U2fBleConnection::ServiceRevision::VERSION_1_1,
      write_callback.GetCallback());
  EXPECT_TRUE(write_callback.WaitForCallback());

  // Expect a successful write of version 1.2.
  SetNextWriteServiceRevisionResponse(device_address, true);
  connection.WriteServiceRevision(
      U2fBleConnection::ServiceRevision::VERSION_1_2,
      write_callback.GetCallback());
  EXPECT_TRUE(write_callback.WaitForCallback());

  // Writing version 1.0 to the bitfield is not intended, so this should fail.
  SetNextWriteServiceRevisionResponse(device_address, true);
  connection.WriteServiceRevision(
      U2fBleConnection::ServiceRevision::VERSION_1_0,
      write_callback.GetCallback());
  EXPECT_FALSE(write_callback.WaitForCallback());

  // Write should always fail on a disconnected device.
  DisconnectDevice(device_address);
  EXPECT_FALSE(connect_callback.WaitForCallback());
  connection.WriteServiceRevision(
      U2fBleConnection::ServiceRevision::VERSION_1_1,
      write_callback.GetCallback());
  EXPECT_FALSE(write_callback.WaitForCallback());
}

}  // namespace device
