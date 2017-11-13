// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <list>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_io_thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "device/base/mock_device_client.h"
#include "device/hid/hid_connection.h"
#include "device/hid/hid_device_filter.h"
#include "device/hid/mock_hid_service.h"
#include "device/test/test_device_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "u2f_apdu_command.h"
#include "u2f_apdu_response.h"
#include "u2f_hid_device.h"
#include "u2f_packet.h"
#include "u2f_register.h"
#include "u2f_request.h"
#include "u2f_sign.h"

namespace device {

class TestSignCallback {
 public:
  TestSignCallback()
      : callback_(base::Bind(&TestSignCallback::ReceivedCallback,
                             base::Unretained(this))) {}
  ~TestSignCallback() {}

  void ReceivedCallback(U2fReturnCode status_code,
                        const std::vector<uint8_t>& response) {
    response_ = std::make_pair(status_code, response);
    closure_.Run();
  }

  std::pair<U2fReturnCode, std::vector<uint8_t>>& WaitForCallback() {
    closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
    return response_;
  }

  const U2fRequest::ResponseCallback& callback() { return callback_; }

 private:
  std::pair<U2fReturnCode, std::vector<uint8_t>> response_;
  base::Closure closure_;
  U2fRequest::ResponseCallback callback_;
  base::RunLoop run_loop_;
};

class TestRegisterCallback {
 public:
  TestRegisterCallback()
      : callback_(base::Bind(&TestRegisterCallback::ReceivedCallback,
                             base::Unretained(this))) {}
  ~TestRegisterCallback() {}

  void ReceivedCallback(U2fReturnCode status_code,
                        const std::vector<uint8_t>& response) {
    response_ = std::make_pair(status_code, response);
    closure_.Run();
  }

  std::pair<U2fReturnCode, std::vector<uint8_t>>& WaitForCallback() {
    closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
    return response_;
  }

  const U2fRequest::ResponseCallback& callback() { return callback_; }

 private:
  std::pair<U2fReturnCode, std::vector<uint8_t>> response_;
  base::Closure closure_;
  U2fRequest::ResponseCallback callback_;
  base::RunLoop run_loop_;
};

class U2fLiveTest : public testing::Test {
 public:
  U2fLiveTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI),
        io_thread_(base::TestIOThread::kAutoStart) {}

  void SetUp() override {}

 private:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::TestIOThread io_thread_;
  TestDeviceClient device_client_;
};

TEST_F(U2fLiveTest, TestBasicSign) {
  std::vector<std::vector<uint8_t>> registered_keys;
  registered_keys.push_back(
      {0x9c, 0x92, 0xbf, 0xd3, 0xf3, 0xb6, 0x2e, 0x00, 0x26, 0x8e, 0x9d,
       0x26, 0xb0, 0x44, 0x5e, 0x7a, 0xbf, 0x64, 0x60, 0x66, 0xda, 0xcc,
       0x14, 0x17, 0xb3, 0x92, 0x10, 0x45, 0x37, 0x8d, 0x1d, 0xca, 0xf8,
       0xf2, 0x92, 0xa6, 0xdd, 0x9f, 0x59, 0x90, 0x62, 0x7f, 0x9e, 0xb3,
       0x80, 0xdc, 0xc5, 0x3e, 0x6b, 0x12, 0x24, 0x80, 0x3a, 0xff, 0x6a,
       0xff, 0x4e, 0x4f, 0xac, 0x8b, 0x95, 0xa6, 0xbf, 0xd8});
  registered_keys.push_back(
      {0xde, 0xa9, 0x0c, 0x4c, 0xe0, 0x2b, 0x49, 0x93, 0x2d, 0xb9, 0xec,
       0xfc, 0x4b, 0xec, 0x1a, 0x4d, 0x10, 0xbf, 0x23, 0x43, 0xc3, 0x41,
       0xe5, 0x34, 0x25, 0xad, 0x0c, 0x3a, 0x23, 0xbe, 0x62, 0xd1, 0xee,
       0xda, 0x47, 0xbb, 0xd9, 0xb3, 0xa8, 0x3f, 0xe0, 0xf9, 0xc8, 0xeb,
       0x26, 0x4f, 0x9e, 0x33, 0x8c, 0x06, 0xf6, 0xec, 0x51, 0x1b, 0x3e,
       0x3a, 0x08, 0x6a, 0x2f, 0x08, 0x15, 0x7e, 0x3e, 0x2d});
  registered_keys.push_back(
      {0xdb, 0x81, 0x75, 0xe1, 0xd7, 0xbf, 0x45, 0xec, 0x72, 0x30, 0x5c, 0x69,
       0x7f, 0xa0, 0xa3, 0xd2, 0xe0, 0xb8, 0xe3, 0xc0, 0x75, 0x87, 0x0c, 0xc5,
       0x6c, 0x05, 0x91, 0xfb, 0x08, 0x81, 0xd1, 0x2f, 0x06, 0xb0, 0x7b, 0x9a,
       0x51, 0x61, 0x26, 0xef, 0x89, 0x7f, 0x17, 0xbf, 0x24, 0x27, 0xc1, 0x35,
       0xda, 0x6e, 0xc9, 0x92, 0x0d, 0xa8, 0x98, 0x75, 0x27, 0xf3, 0x36, 0xdd,
       0x2a, 0x99, 0x2a, 0xcd, 0x71, 0xde, 0x5f, 0xeb, 0x0d, 0x3b, 0x23, 0x31,
       0xfe, 0x58, 0x3d, 0x0e, 0x36, 0x54, 0x03, 0x7a});
  std::vector<uint8_t> challenge_hash = {
      0x41, 0x42, 0xd2, 0x1c, 0x00, 0xd9, 0x4f, 0xfb, 0x9d, 0x50, 0x4a,
      0xda, 0x8f, 0x99, 0xb7, 0x21, 0xf4, 0xb1, 0x91, 0xae, 0x4e, 0x37,
      0xca, 0x01, 0x40, 0xf6, 0x96, 0xb6, 0x98, 0x3c, 0xfa, 0xcb};
  std::vector<uint8_t> app_hash = {
      0xf0, 0xe6, 0xa6, 0xa9, 0x70, 0x42, 0xa4, 0xf1, 0xf1, 0xc8, 0x7f,
      0x5f, 0x7d, 0x44, 0x31, 0x5b, 0x2d, 0x85, 0x2c, 0x2d, 0xf5, 0xc7,
      0x99, 0x1c, 0xc6, 0x62, 0x41, 0xbf, 0x70, 0x72, 0xd1, 0xc4};

  TestSignCallback cb;

  std::unique_ptr<U2fRequest> request = U2fSign::TrySign(
      registered_keys, challenge_hash, app_hash, cb.callback());

  std::pair<U2fReturnCode, std::vector<uint8_t>> response_pair =
      cb.WaitForCallback();
  printf("Response code %x\n", static_cast<uint8_t>(response_pair.first));
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

TEST_F(U2fLiveTest, TestBasicRegistration) {
  std::vector<uint8_t> challenge_hash = {
      0x41, 0x42, 0xd2, 0x1c, 0x00, 0xd9, 0x4f, 0xfb, 0x9d, 0x50, 0x4a,
      0xda, 0x8f, 0x99, 0xb7, 0x21, 0xf4, 0xb1, 0x91, 0xae, 0x4e, 0x37,
      0xca, 0x01, 0x40, 0xf6, 0x96, 0xb6, 0x98, 0x3c, 0xfa, 0xcb};
  std::vector<uint8_t> app_hash = {
      0xf0, 0xe6, 0xa6, 0xa9, 0x70, 0x42, 0xa4, 0xf1, 0xf1, 0xc8, 0x7f,
      0x5f, 0x7d, 0x44, 0x31, 0x5b, 0x2d, 0x85, 0x2c, 0x2d, 0xf5, 0xc7,
      0x99, 0x1c, 0xc6, 0x62, 0x41, 0xbf, 0x70, 0x72, 0xd1, 0xc4};

  TestRegisterCallback cb;

  std::unique_ptr<U2fRequest> request =
      U2fRegister::TryRegistration(challenge_hash, app_hash, cb.callback());

  //  U2fRegister registration_request(challenge_hash, app_hash, cb.callback());
  //  registration_request.Start();
  std::pair<U2fReturnCode, std::vector<uint8_t>> response_pair =
      cb.WaitForCallback();
  std::vector<uint8_t> response = response_pair.second;

  printf("Length: %d\nReserved byte %x\n", (int)response.size(), response[0]);
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
