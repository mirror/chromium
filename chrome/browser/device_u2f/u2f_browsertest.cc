// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/run_loop.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "device/u2f/mock_u2f_discovery.h"
#include "device/u2f/u2f_ble_device.h"
#include "device/u2f/u2f_ble_discovery.h"
#include "net/cert/x509_certificate.h"
#include "net/der/input.h"
#include "net/der/parser.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include <iostream>

namespace {

ACTION_P(ReturnFromAsyncCall, closure) {
  closure.Run();
}

char HexToChar(uint8_t hex) {
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
  explicit U2fRegisterResponse(const std::vector<uint8_t>& data) {
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

  const std::vector<uint8_t> key_handle() const { return key_handle_; }

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
  std::vector<uint8_t> key_handle_;
  scoped_refptr<net::X509Certificate> certificate_;
  std::vector<uint8_t> signature_;

  DISALLOW_COPY_AND_ASSIGN(U2fRegisterResponse);
};

class U2fSignResponse {
 public:
  explicit U2fSignResponse(const std::vector<uint8_t>& data) {
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
  std::vector<uint8_t> signature_;

  DISALLOW_COPY_AND_ASSIGN(U2fSignResponse);
};

}  // namespace

namespace device {

namespace {

class TestU2fDiscoveryDelegate : public MockU2fDiscovery::MockDelegate {
 public:
  void OnDeviceAdded(std::unique_ptr<U2fDevice> device) override {
    if (!device_added())
      device_ = std::move(device);
    // MockU2fDiscovery::MockDelegate::OnDeviceAdded(nullptr);
  }

  bool device_added() const { return !!device_; }

  std::unique_ptr<U2fDevice> take_device() { return std::move(device_); }

 private:
  std::unique_ptr<U2fDevice> device_;
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
  TestU2fDiscoveryDelegate delegate;

  U2fBleDiscovery discovery;
  discovery.SetDelegate(delegate.GetWeakPtr());

  {
    base::RunLoop run_loop;
    auto quit = run_loop.QuitClosure();
    ON_CALL(delegate, OnDeviceAddedStr(::testing::_))
        .WillByDefault(::testing::Return());
    EXPECT_CALL(delegate, OnStarted(true)).WillOnce(ReturnFromAsyncCall(quit));
    discovery.Start();
    run_loop.Run();
  }

  if (!delegate.device_added()) {
    base::RunLoop run_loop;
    auto quit = run_loop.QuitClosure();
    EXPECT_CALL(delegate, OnDeviceAddedStr(::testing::_))
        .WillOnce(ReturnFromAsyncCall(quit));
    LOG(INFO) << "Waiting for U2F BLE devices.";
    run_loop.Run();
  }

  {
    base::RunLoop run_loop;
    auto quit = run_loop.QuitClosure();
    EXPECT_CALL(delegate, OnStopped(true)).WillOnce(ReturnFromAsyncCall(quit));
    discovery.Stop();
    run_loop.Run();
  }

  // ===========================================================================

  std::unique_ptr<U2fBleDevice> device(
      static_cast<U2fBleDevice*>(delegate.take_device().release()));

  LOG(INFO) << "Using device: " << device->GetId();

  device->SendPing(
      std::vector<uint8_t>(),
      base::Bind([](std::vector<uint8_t> data) { LOG(INFO) << Format(data); }));
  device->SendPing(
      std::vector<uint8_t>(1, 'a'),
      base::Bind([](std::vector<uint8_t> data) { LOG(INFO) << Format(data); }));
  device->SendPing(
      std::vector<uint8_t>(10, 'b'),
      base::Bind([](std::vector<uint8_t> data) { LOG(INFO) << Format(data); }));
  device->SendPing(
      std::vector<uint8_t>(20, 'c'),
      base::Bind([](std::vector<uint8_t> data) { LOG(INFO) << Format(data); }));
  device->SendPing(
      std::vector<uint8_t>(567, 'd'),
      base::Bind([](std::vector<uint8_t> data) { LOG(INFO) << Format(data); }));

  std::vector<uint8_t> key_handle;

  {
    base::RunLoop run_loop;
    auto quit = run_loop.QuitClosure();

    device->Register(
        std::vector<uint8_t>(32), std::vector<uint8_t>(32),
        base::Bind(
            [](base::Closure quit, std::vector<uint8_t>* key_handle,
               U2fReturnCode code, const std::vector<uint8_t>& data) {
              LOG(INFO) << "response code = " << int(code);
              if (code != U2fReturnCode::SUCCESS)
                return;

              U2fRegisterResponse response(data);
              response.Print();
              *key_handle = response.key_handle();

              quit.Run();
            },
            quit, &key_handle));
    run_loop.Run();
  }
  {
    base::RunLoop run_loop;
    auto quit = run_loop.QuitClosure();

    device->Sign(std::vector<uint8_t>(32), std::vector<uint8_t>(32), key_handle,
                 base::Bind(
                     [](base::Closure quit, U2fReturnCode code,
                        const std::vector<uint8_t>& data) {
                       LOG(INFO) << "response code = " << int(code);
                       if (code != U2fReturnCode::SUCCESS)
                         return;

                       U2fSignResponse response(data);
                       response.Print();

                       quit.Run();
                     },
                     quit));
    run_loop.Run();
  }
}

}  // namespace device
