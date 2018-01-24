// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/base64.h"
#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/bind_test_util.h"
#include "base/threading/platform_thread.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "device/bluetooth/bluetooth_adapter.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/u2f/mock_u2f_discovery.h"
#include "device/u2f/u2f_ble_device.h"
#include "device/u2f/u2f_ble_discovery.h"
#include "net/cert/x509_certificate.h"
#include "net/der/input.h"
#include "net/der/parser.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include <iostream>

using Bytes = std::vector<uint8_t>;
using ::testing::_;

namespace {

ACTION_P(ReturnFromAsyncCall, closure) {
  closure.Run();
}

constexpr char HexToChar(uint8_t hex) {
  return hex < 10 ? '0' + hex : 'A' + (hex - 10);
}

template <typename Data>
std::string Format(const Data& data) {
  std::string result;
  for (uint8_t byte : data) {
    result.push_back(HexToChar((byte >> 4) & 0xF));
    result.push_back(HexToChar(byte & 0xF));
    result.push_back(' ');
  }
  return result;
}

class U2fRegisterResponse {
 public:
  explicit U2fRegisterResponse(const Bytes& data) {
    size_t position = 0;

    reserved_ = data[position++];
    for (size_t i = 0; i < user_public_key_.size(); ++i) {
      user_public_key_[i] = data[position++];
    }

    uint8_t key_handle_length = data[position++];
    key_handle_.assign(&data[position], &data[position] + key_handle_length);
    position += key_handle_length;

    net::der::Input cert_with_trailing_bytes(&data[position],
                                             data.size() - position);
    net::der::Parser parser(cert_with_trailing_bytes);
    net::der::Input cert_bytes;
    CHECK(parser.ReadRawTLV(&cert_bytes));
    certificate_ = net::X509Certificate::CreateFromBytes(
        reinterpret_cast<const char*>(cert_bytes.UnsafeData()),
        cert_bytes.Length());

    position += cert_bytes.Length();
    signature_.assign(&data[position], data.data() + data.size());
  }

  const Bytes& key_handle() const { return key_handle_; }

  void Print() {
    std::cerr << "=== U2F register response ===\n"
              << "Reserved: " << static_cast<int>(reserved_) << "\n"
              << "PubKey: " << Format(user_public_key_) << "\n"
              << "KeyHandle: " << Format(key_handle_) << "\n";

    std::vector<std::string> pem_encoded;
    certificate_->GetPEMEncodedChain(&pem_encoded);
    for (const auto& encoded : pem_encoded)
      std::cerr << "Cert: --->\n" << encoded;
    std::cerr << "Signature: " << Format(signature_) << std::endl;
  }

 private:
  uint8_t reserved_;
  std::array<uint8_t, 65> user_public_key_;
  Bytes key_handle_;
  scoped_refptr<net::X509Certificate> certificate_;
  Bytes signature_;

  DISALLOW_COPY_AND_ASSIGN(U2fRegisterResponse);
};

class U2fSignResponse {
 public:
  explicit U2fSignResponse(const Bytes& data) {
    size_t position = 0;

    user_presence_ = data[position++];
    for (int i = 0; i < 4; ++i)
      counter_ |= data[position++] << (3 - i) * 8;
    signature_.assign(&data[position], data.data() + data.size());
  }

  void Print() {
    std::cerr << "=== U2F sign response ===\n"
              << "User Presence: " << static_cast<int>(user_presence_) << "\n"
              << "Counter: " << counter_ << "\n"
              << "Signature: " << Format(signature_) << "\n";
  }

 private:
  uint8_t user_presence_ = 0;
  uint32_t counter_ = 0;
  Bytes signature_;

  DISALLOW_COPY_AND_ASSIGN(U2fSignResponse);
};

}  // namespace

namespace device {

namespace {

constexpr char kCommandPing[] = "ping";
constexpr char kCommandRegister[] = "register";
constexpr char kCommandSign[] = "sign";
constexpr char kParameterPIN[] = "pin";
constexpr char kParameterKeyHandle[] = "key";

class TestU2fDiscoveryObserver : public MockU2fDiscoveryObserver {
 public:
  void DeviceAdded(U2fDiscovery* discovery, U2fDevice* device) override {
    if (!device_added())
      device_ = device;
    MockU2fDiscoveryObserver::DeviceAdded(discovery, device);
  }

  bool device_added() const { return device_; }

  U2fDevice* device() { return device_; }

 private:
  U2fDevice* device_ = nullptr;
};

class TestBlePairingOrchestrator
    : public device::BluetoothDevice::PairingDelegate {
 public:
  TestBlePairingOrchestrator(device::BluetoothDevice* device,
                             base::OnceClosure callback,
                             size_t max_retries = 5u)
      : device_(device),
        callback_(std::move(callback)),
        remaining_retries_(max_retries) {}

  ~TestBlePairingOrchestrator() override { LOG(ERROR) << "DTOR"; }

  void PairWithRetries() {
    device_->Pair(this,
                  base::BindRepeating(&TestBlePairingOrchestrator::OnSuccess,
                                      base::Unretained(this)),
                  base::BindRepeating(&TestBlePairingOrchestrator::OnFailure,
                                      base::Unretained(this)));
  }

 protected:
  void OnSuccess() {
    LOG(INFO) << "Successfully paired: " << device_->GetAddress();

    DCHECK(callback_);
    std::move(callback_).Run();
  }

  void OnFailure(device::BluetoothDevice::ConnectErrorCode error) {
    LOG(INFO) << "Pairing error for device " << device_->GetAddress() << ": "
              << static_cast<int>(error);

    if (remaining_retries_--) {
      base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
          FROM_HERE,
          base::BindOnce(&TestBlePairingOrchestrator::PairWithRetries,
                         base::Unretained(this)),
          base::TimeDelta::FromSeconds(3));
    } else {
      DCHECK(callback_);
      std::move(callback_).Run();
    }
  }

  // PairingDelegate:
  void RequestPinCode(BluetoothDevice* device) override { NOTREACHED(); }

  void DisplayPinCode(BluetoothDevice* device,
                      const std::string& pincode) override {
    NOTREACHED();
  }

  void DisplayPasskey(BluetoothDevice* device, uint32_t passkey) override {
    NOTREACHED();
  }

  void KeysEntered(BluetoothDevice* device, uint32_t entered) override {
    NOTREACHED();
  }

  void ConfirmPasskey(BluetoothDevice* device, uint32_t passkey) override {
    NOTREACHED();
  }

  void AuthorizePairing(BluetoothDevice* device) override { NOTREACHED(); }

  void RequestPasskey(BluetoothDevice* device) override {
    DCHECK_EQ(device, device_);
    DCHECK(device->ExpectingPasskey());

    int pin = 0;
    ASSERT_TRUE(base::StringToInt(
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            kParameterPIN),
        &pin));

    LOG(INFO) << "RequestPasskey, supplying passkey: " << pin;
    device->SetPasskey(pin);
  }

 private:
  device::BluetoothDevice* device_;
  base::OnceClosure callback_;
  size_t remaining_retries_;

  DISALLOW_COPY_AND_ASSIGN(TestBlePairingOrchestrator);
};

}  // namespace

class U2fBrowserTest : public InProcessBrowserTest {
 public:
  U2fBrowserTest() {}
  ~U2fBrowserTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(U2fBrowserTest);
};

IN_PROC_BROWSER_TEST_F(U2fBrowserTest, U2fViaBluetooth) {
  auto* command_line = base::CommandLine::ForCurrentProcess();

  TestU2fDiscoveryObserver observer;
  U2fBleDiscovery discovery;
  discovery.AddObserver(&observer);

  {
    base::RunLoop run_loop;
    auto quit = run_loop.QuitClosure();
    ON_CALL(observer, DeviceAdded(_, _)).WillByDefault(::testing::Return());
    EXPECT_CALL(observer, DiscoveryStarted(_, true))
        .WillOnce(ReturnFromAsyncCall(quit));
    discovery.Start();
    run_loop.Run();
  }

  if (!observer.device_added()) {
    base::RunLoop run_loop;
    auto quit = run_loop.QuitClosure();
    EXPECT_CALL(observer, DeviceAdded(_, _))
        .WillOnce(ReturnFromAsyncCall(quit));
    LOG(INFO) << "Waiting for U2F BLE devices.";
    run_loop.Run();
  }

  {
    base::RunLoop run_loop;
    auto quit = run_loop.QuitClosure();
    EXPECT_CALL(observer, DiscoveryStopped(_, true))
        .WillOnce(ReturnFromAsyncCall(quit));
    discovery.Stop();
    run_loop.Run();
  }

  // ===========================================================================

  U2fBleDevice* device = static_cast<U2fBleDevice*>(observer.device());
  LOG(INFO) << "Using device: " << device->GetId();

  device::BluetoothDevice* ble_device = device->GetDevice();

  LOG(INFO) << "Device with address " << ble_device->GetAddress()
            << " has pairing state: " << ble_device->IsPaired();

  if (!ble_device->IsPaired()) {
    base::RunLoop run_loop;
    LOG(INFO) << "Pairing device ...";
    TestBlePairingOrchestrator pairer(ble_device, run_loop.QuitClosure());
    pairer.PairWithRetries();
    run_loop.Run();
  }

  Bytes key_handle;
  if (command_line->HasSwitch(kCommandRegister)) {
    base::RunLoop run_loop;
    device->Register(
        Bytes(32), Bytes(32),
        base::BindLambdaForTesting([&](U2fReturnCode code, const Bytes& data) {
          LOG(INFO) << "response code = " << int(code);
          if (code != U2fReturnCode::SUCCESS)
            return;

          U2fRegisterResponse response(data);
          response.Print();
          key_handle = response.key_handle();

          run_loop.Quit();
        }));
    run_loop.Run();

    std::string key_handle_str;
    key_handle_str.assign(
        reinterpret_cast<const char*>(key_handle.data()),
        reinterpret_cast<const char*>(key_handle.data()) + key_handle.size());

    std::string key_handle_base64;
    base::Base64Encode(key_handle_str, &key_handle_base64);
    LOG(INFO) << "Key handle = " << key_handle_base64;
  }

  if (command_line->HasSwitch(kCommandSign)) {
    base::RunLoop run_loop;
    device->Sign(
        Bytes(32), Bytes(32), key_handle,
        base::BindLambdaForTesting([&](U2fReturnCode code, const Bytes& data) {
          LOG(INFO) << "response code = " << int(code);
          if (code != U2fReturnCode::SUCCESS)
            return;

          U2fSignResponse response(data);
          response.Print();

          run_loop.Quit();
        }));
    run_loop.Run();
  }

  LOG(INFO) << "Device with address " << ble_device->GetAddress()
            << " has pairing state: " << ble_device->IsPaired();

  if (command_line->HasSwitch(kCommandPing)) {
    device->SendPing(
        Bytes(), base::BindOnce([](U2fReturnCode code, const Bytes& data) {
          LOG(INFO) << (code == U2fReturnCode::SUCCESS ? "SUCCESS" : "FAILURE");
          LOG(INFO) << Format(data);
        }));
    device->SendPing(
        Bytes(1, 'a'),
        base::BindOnce([](U2fReturnCode code, const Bytes& data) {
          LOG(INFO) << (code == U2fReturnCode::SUCCESS ? "SUCCESS" : "FAILURE");
          LOG(INFO) << Format(data);
        }));
    device->SendPing(
        Bytes(10, 'b'),
        base::BindOnce([](U2fReturnCode code, const Bytes& data) {
          LOG(INFO) << (code == U2fReturnCode::SUCCESS ? "SUCCESS" : "FAILURE");
          LOG(INFO) << Format(data);
        }));
    device->SendPing(
        Bytes(20, 'c'),
        base::BindOnce([](U2fReturnCode code, const Bytes& data) {
          LOG(INFO) << (code == U2fReturnCode::SUCCESS ? "SUCCESS" : "FAILURE");
          LOG(INFO) << Format(data);
        }));
    device->SendPing(
        Bytes(567, 'd'),
        base::BindOnce([](U2fReturnCode code, const Bytes& data) {
          LOG(INFO) << (code == U2fReturnCode::SUCCESS ? "SUCCESS" : "FAILURE");
          LOG(INFO) << Format(data);
        }));
  }

  if (command_line->HasSwitch(kParameterKeyHandle)) {
    std::string key_handle_str;
    ASSERT_TRUE(base::Base64Decode(
        command_line->GetSwitchValueASCII(kParameterKeyHandle),
        &key_handle_str));
    key_handle.assign(reinterpret_cast<const uint8_t*>(key_handle_str.data()),
                      reinterpret_cast<const uint8_t*>(key_handle_str.data()) +
                          key_handle_str.size());
  }

  if (command_line->HasSwitch(kCommandRegister)) {
    base::RunLoop run_loop;
    device->Register(
        Bytes(32), Bytes(32),
        base::BindLambdaForTesting([&](U2fReturnCode code, const Bytes& data) {
          LOG(INFO) << "response code = " << int(code);
          if (code != U2fReturnCode::SUCCESS)
            return;

          U2fRegisterResponse response(data);
          response.Print();
          key_handle = response.key_handle();

          run_loop.Quit();
        }));
    run_loop.Run();

    std::string key_handle_str;
    key_handle_str.assign(
        reinterpret_cast<const char*>(key_handle.data()),
        reinterpret_cast<const char*>(key_handle.data()) + key_handle.size());

    std::string key_handle_base64;
    base::Base64Encode(key_handle_str, &key_handle_base64);
    LOG(INFO) << "KEY HANDLE = " << key_handle_base64;
  }

  if (command_line->HasSwitch(kCommandSign)) {
    base::RunLoop run_loop;
    device->Sign(
        Bytes(32), Bytes(32), key_handle,
        base::BindLambdaForTesting([&](U2fReturnCode code, const Bytes& data) {
          LOG(INFO) << "response code = " << int(code);
          if (code != U2fReturnCode::SUCCESS)
            return;

          U2fSignResponse response(data);
          response.Print();

          run_loop.Quit();
        }));
    run_loop.Run();
  }
}

}  // namespace device
