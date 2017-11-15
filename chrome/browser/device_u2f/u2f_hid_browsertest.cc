// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/common/service_manager_connection.h"
#include "device/u2f/u2f_hid_discovery.h"
#include "device/u2f/u2f_register.h"
#include "device/u2f/u2f_sign.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

class TestRegisterCallback {
 public:
  TestRegisterCallback()
      : callback_(base::Bind(&TestRegisterCallback::ReceivedCallback,
                             base::Unretained(this))) {}
  ~TestRegisterCallback() {}

  void ReceivedCallback(device::U2fReturnCode status_code,
                        const std::vector<uint8_t>& response) {
    response_ = std::make_pair(status_code, response);
    closure_.Run();
  }

  std::pair<device::U2fReturnCode, std::vector<uint8_t>>& WaitForCallback() {
    closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
    return response_;
  }

  const device::U2fRequest::ResponseCallback& callback() { return callback_; }

 private:
  std::pair<device::U2fReturnCode, std::vector<uint8_t>> response_;
  base::Closure closure_;
  device::U2fRequest::ResponseCallback callback_;
  base::RunLoop run_loop_;
};

class TestSignCallback {
 public:
  TestSignCallback()
      : callback_(base::Bind(&TestSignCallback::ReceivedCallback,
                             base::Unretained(this))) {}
  ~TestSignCallback() {}

  void ReceivedCallback(device::U2fReturnCode status_code,
                        const std::vector<uint8_t>& response) {
    response_ = std::make_pair(status_code, response);
    closure_.Run();
  }

  std::pair<device::U2fReturnCode, std::vector<uint8_t>>& WaitForCallback() {
    closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
    return response_;
  }

  const device::U2fRequest::ResponseCallback& callback() { return callback_; }

 private:
  std::pair<device::U2fReturnCode, std::vector<uint8_t>> response_;
  base::Closure closure_;
  device::U2fRequest::ResponseCallback callback_;
  base::RunLoop run_loop_;
};

class U2fHidBrowserTest : public InProcessBrowserTest {
 public:
  U2fHidBrowserTest() {}
  ~U2fHidBrowserTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(U2fHidBrowserTest);
};

IN_PROC_BROWSER_TEST_F(U2fHidBrowserTest, TestBasicSign) {
  std::vector<std::vector<uint8_t>> registered_keys;
  registered_keys.push_back(
      {0x0c, 0x33, 0xdf, 0x8f, 0xe3, 0x31, 0x64, 0xef, 0x17, 0x62, 0xa7, 0xa3,
       0xec, 0x30, 0x54, 0x51, 0xcb, 0x77, 0xfb, 0x54, 0xb6, 0xca, 0x20, 0x0c,
       0x31, 0x4c, 0x6b, 0x85, 0x07, 0x0d, 0xec, 0xa7, 0xf0, 0x58, 0xed, 0xe3,
       0xac, 0x02, 0x52, 0x62, 0xda, 0x3e, 0xc8, 0x3f, 0x97, 0x84, 0x38, 0x42,
       0x73, 0xc9, 0x31, 0x9f, 0x78, 0x5d, 0xf2, 0xb6, 0xa2, 0xe5, 0xcc, 0x27,
       0x39, 0x3e, 0x54, 0x96, 0x7a, 0x62, 0x43, 0x13, 0x36, 0x0d, 0x06, 0x50,
       0x83, 0x3c, 0x05, 0xcc, 0xc0, 0x28, 0x4c, 0x22});

  std::vector<uint8_t> challenge_hash = {
      0x41, 0x42, 0xd2, 0x1c, 0x00, 0xd9, 0x4f, 0xfb, 0x9d, 0x50, 0x4a,
      0xda, 0x8f, 0x99, 0xb7, 0x21, 0xf4, 0xb1, 0x91, 0xae, 0x4e, 0x37,
      0xca, 0x01, 0x40, 0xf6, 0x96, 0xb6, 0x98, 0x3c, 0xfa, 0xcb};
  std::vector<uint8_t> app_hash = {
      0xf0, 0xe6, 0xa6, 0xa9, 0x70, 0x42, 0xa4, 0xf1, 0xf1, 0xc8, 0x7f,
      0x5f, 0x7d, 0x44, 0x31, 0x5b, 0x2d, 0x85, 0x2c, 0x2d, 0xf5, 0xc7,
      0x99, 0x1c, 0xc6, 0x62, 0x41, 0xbf, 0x70, 0x72, 0xd1, 0xc4};

  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();

  std::unique_ptr<device::U2fDiscovery> discovery =
      std::make_unique<device::U2fHidDiscovery>(connector);

  std::vector<std::unique_ptr<device::U2fDiscovery>> discoveries;
  discoveries.push_back(std::move(discovery));

  TestSignCallback cb;
  std::unique_ptr<device::U2fRequest> request =
      device::U2fSign::TrySign(registered_keys, challenge_hash, app_hash,
                               std::move(discoveries), cb.callback());

  request->Start();
  std::pair<device::U2fReturnCode, std::vector<uint8_t>> response_pair =
      cb.WaitForCallback();
  printf("\n\n\n\n\nResponse code %x\n",
         static_cast<uint8_t>(response_pair.first));
  std::vector<uint8_t> response = response_pair.second;

  if (response.size() > 0) {
    if (response[0] == 5) {
      printf("Fake enroll response received\n");
      return;
    }
    printf("User presence: %u ", response[0]);
    uint32_t counter = 0;
    counter |= response[1] << 24;
    counter |= response[2] << 16;
    counter |= response[3] << 8;
    counter |= response[4];
    printf("Counter: %d\n", counter);

    printf("Signature: [%lu bytes] \n", response.size() - 5);
    for (size_t i = 5; i < response.size(); i++) {
      printf("%02x ", response[i]);
    }
    printf("\n");
  }
}

IN_PROC_BROWSER_TEST_F(U2fHidBrowserTest, TestBasicRegistration) {
  std::vector<uint8_t> challenge_hash = {
      0x41, 0x42, 0xd2, 0x1c, 0x00, 0xd9, 0x4f, 0xfb, 0x9d, 0x50, 0x4a,
      0xda, 0x8f, 0x99, 0xb7, 0x21, 0xf4, 0xb1, 0x91, 0xae, 0x4e, 0x37,
      0xca, 0x01, 0x40, 0xf6, 0x96, 0xb6, 0x98, 0x3c, 0xfa, 0xcb};
  std::vector<uint8_t> app_hash = {
      0xf0, 0xe6, 0xa6, 0xa9, 0x70, 0x42, 0xa4, 0xf1, 0xf1, 0xc8, 0x7f,
      0x5f, 0x7d, 0x44, 0x31, 0x5b, 0x2d, 0x85, 0x2c, 0x2d, 0xf5, 0xc7,
      0x99, 0x1c, 0xc6, 0x62, 0x41, 0xbf, 0x70, 0x72, 0xd1, 0xc4};

  std::vector<std::vector<uint8_t>> registered_keys = {};

  TestRegisterCallback cb;

  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();

  std::unique_ptr<device::U2fDiscovery> discovery =
      std::make_unique<device::U2fHidDiscovery>(connector);

  std::vector<std::unique_ptr<device::U2fDiscovery>> discoveries;
  discoveries.push_back(std::move(discovery));

  std::unique_ptr<device::U2fRequest> request =
      device::U2fRegister::TryRegistration(registered_keys, challenge_hash,
                                           app_hash, std::move(discoveries),
                                           cb.callback());

  std::pair<device::U2fReturnCode, std::vector<uint8_t>> response_pair =
      cb.WaitForCallback();
  std::vector<uint8_t> response = response_pair.second;

  printf("\n\n\n\n\nResponse code %x\n",
         static_cast<uint8_t>(response_pair.first));
  printf("Length: %d\nReserved byte %x\n", static_cast<int>(response.size()),
         response[0]);
  std::vector<uint8_t> user_pub(response.begin() + 1, response.begin() + 66);
  printf("User Public Key:\n");
  for (size_t i = 0; i < user_pub.size(); i++) {
    printf("0x%02x, ", user_pub[i]);
  }
  printf("\n");

  uint8_t key_length = response[66];
  printf("key length: %d\n", key_length);

  std::vector<uint8_t> key(response.begin() + 67,
                           response.begin() + 67 + key_length);
  for (size_t i = 0; i < key.size(); i++) {
    printf("0x%02x, ", key[i]);
  }
  printf("\n");
}

IN_PROC_BROWSER_TEST_F(U2fHidBrowserTest, TestDuplicateRegistration) {
  std::vector<uint8_t> challenge_hash = {
      0x41, 0x42, 0xd2, 0x1c, 0x00, 0xd9, 0x4f, 0xfb, 0x9d, 0x50, 0x4a,
      0xda, 0x8f, 0x99, 0xb7, 0x21, 0xf4, 0xb1, 0x91, 0xae, 0x4e, 0x37,
      0xca, 0x01, 0x40, 0xf6, 0x96, 0xb6, 0x98, 0x3c, 0xfa, 0xcb};
  std::vector<uint8_t> app_hash = {
      0xf0, 0xe6, 0xa6, 0xa9, 0x70, 0x42, 0xa4, 0xf1, 0xf1, 0xc8, 0x7f,
      0x5f, 0x7d, 0x44, 0x31, 0x5b, 0x2d, 0x85, 0x2c, 0x2d, 0xf5, 0xc7,
      0x99, 0x1c, 0xc6, 0x62, 0x41, 0xbf, 0x70, 0x72, 0xd1, 0xc4};
  std::vector<uint8_t> duplicate_key = {
      0x0c, 0x33, 0xdf, 0x8f, 0xe3, 0x31, 0x64, 0xef, 0x17, 0x62, 0xa7, 0xa3,
      0xec, 0x30, 0x54, 0x51, 0xcb, 0x77, 0xfb, 0x54, 0xb6, 0xca, 0x20, 0x0c,
      0x31, 0x4c, 0x6b, 0x85, 0x07, 0x0d, 0xec, 0xa7, 0xf0, 0x58, 0xed, 0xe3,
      0xac, 0x02, 0x52, 0x62, 0xda, 0x3e, 0xc8, 0x3f, 0x97, 0x84, 0x38, 0x42,
      0x73, 0xc9, 0x31, 0x9f, 0x78, 0x5d, 0xf2, 0xb6, 0xa2, 0xe5, 0xcc, 0x27,
      0x39, 0x3e, 0x54, 0x96, 0x7a, 0x62, 0x43, 0x13, 0x36, 0x0d, 0x06, 0x50,
      0x83, 0x3c, 0x05, 0xcc, 0xc0, 0x28, 0x4c, 0x22};
  std::vector<std::vector<uint8_t>> registered_keys = {};
  registered_keys.push_back(duplicate_key);

  TestRegisterCallback cb;

  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();

  std::unique_ptr<device::U2fDiscovery> discovery =
      std::make_unique<device::U2fHidDiscovery>(connector);

  std::vector<std::unique_ptr<device::U2fDiscovery>> discoveries;
  discoveries.push_back(std::move(discovery));

  std::unique_ptr<device::U2fRequest> request =
      device::U2fRegister::TryRegistration(registered_keys, challenge_hash,
                                           app_hash, std::move(discoveries),
                                           cb.callback());

  std::pair<device::U2fReturnCode, std::vector<uint8_t>> response_pair =
      cb.WaitForCallback();
  std::vector<uint8_t> response = response_pair.second;
  printf("\n\n\n\n\nResponse code %x\n",
         static_cast<uint8_t>(response_pair.first));
  printf("Length: %d\nReserved byte %x\n", static_cast<int>(response.size()),
         response[0]);
  std::vector<uint8_t> user_pub(response.begin() + 1, response.begin() + 66);
  printf("User Public Key:\n");
  for (size_t i = 0; i < user_pub.size(); i++) {
    printf("0x%02x, ", user_pub[i]);
  }
  printf("\n");

  uint8_t key_length = response[66];
  printf("key length: %d\n", key_length);

  std::vector<uint8_t> key(response.begin() + 67,
                           response.begin() + 67 + key_length);
  for (size_t i = 0; i < key.size(); i++) {
    printf("0x%02x, ", key[i]);
  }
  printf("\n");
}

}  // namespace device
