// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <list>

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_io_thread.h"
#include "device/base/mock_device_client.h"
#include "device/hid/mock_hid_service.h"
#include "device/test/test_device_client.h"
#include "mock_u2f_device.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "u2f_register.h"

namespace device {

class U2fRegisterTest : public testing::Test {
 public:
  U2fRegisterTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI),
        io_thread_(base::TestIOThread::kAutoStart) {}

  void SetUp() override {
    MockHidService* hid_service = device_client_.hid_service();
    hid_service->FirstEnumerationComplete();
  }

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::TestIOThread io_thread_;
  device::MockDeviceClient device_client_;
};

class TestRegisterCallback {
 public:
  TestRegisterCallback()
      : callback_(base::Bind(&TestRegisterCallback::ReceivedCallback,
                             base::Unretained(this))) {}
  ~TestRegisterCallback() {}

  void ReceivedCallback(U2fReturnCode status_code,
                        const std::vector<uint8_t>& response,
                        const std::vector<uint8_t>& key_handle) {
    response_ = std::make_tuple(status_code, response, key_handle);
    closure_.Run();
  }

  std::tuple<U2fReturnCode, std::vector<uint8_t>, std::vector<uint8_t>>&
  WaitForCallback() {
    closure_ = run_loop_.QuitClosure();
    run_loop_.Run();
    return response_;
  }

  const U2fRequest::ResponseCallback& callback() { return callback_; }

 private:
  std::tuple<U2fReturnCode, std::vector<uint8_t>, std::vector<uint8_t>>
      response_;
  base::Closure closure_;
  U2fRequest::ResponseCallback callback_;
  base::RunLoop run_loop_;
};

TEST_F(U2fRegisterTest, TestRegisterSuccess) {
  std::unique_ptr<MockU2fDevice> device(new MockU2fDevice());
  EXPECT_CALL(*device.get(), DeviceTransactPtr(testing::_, testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));
  EXPECT_CALL(*device.get(), TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  TestRegisterCallback cb;
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      std::vector<uint8_t>(32), std::vector<uint8_t>(32), cb.callback());
  request->Start();
  request->AddDeviceForTesting(std::move(device));
  std::tuple<U2fReturnCode, std::vector<uint8_t>, std::vector<uint8_t>>&
      response = cb.WaitForCallback();

  EXPECT_EQ(U2fReturnCode::SUCCESS, std::get<0>(response));
  ASSERT_GE(1u, std::get<1>(response).size());
  EXPECT_EQ(static_cast<uint8_t>(MockU2fDevice::kRegister),
            std::get<1>(response)[0]);

  // Verify that we get a blank key handle.
  EXPECT_TRUE(std::get<2>(response).empty());
}

TEST_F(U2fRegisterTest, TestDelayedSuccess) {
  std::unique_ptr<MockU2fDevice> device(new MockU2fDevice());

  // Go through the state machine twice before success
  EXPECT_CALL(*device.get(), DeviceTransactPtr(testing::_, testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::NotSatisfied))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));
  EXPECT_CALL(*device.get(), TryWink(testing::_))
      .Times(2)
      .WillRepeatedly(testing::Invoke(MockU2fDevice::WinkDoNothing));
  TestRegisterCallback cb;

  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      std::vector<uint8_t>(32), std::vector<uint8_t>(32), cb.callback());
  request->Start();
  request->AddDeviceForTesting(std::move(device));
  std::tuple<U2fReturnCode, std::vector<uint8_t>, std::vector<uint8_t>>&
      response = cb.WaitForCallback();

  EXPECT_EQ(U2fReturnCode::SUCCESS, std::get<0>(response));
  ASSERT_GE(1u, std::get<1>(response).size());
  EXPECT_EQ(static_cast<uint8_t>(MockU2fDevice::kRegister),
            std::get<1>(response)[0]);

  // Verify that we get a blank key handle.
  EXPECT_TRUE(std::get<2>(response).empty());
}

TEST_F(U2fRegisterTest, TestMultipleDevices) {
  // Second device will have a successful touch
  std::unique_ptr<MockU2fDevice> device0(new MockU2fDevice());
  std::unique_ptr<MockU2fDevice> device1(new MockU2fDevice());

  EXPECT_CALL(*device0.get(), DeviceTransactPtr(testing::_, testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::NotSatisfied));
  // One wink per device
  EXPECT_CALL(*device0.get(), TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(*device1.get(), DeviceTransactPtr(testing::_, testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));
  EXPECT_CALL(*device1.get(), TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));

  TestRegisterCallback cb;
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      std::vector<uint8_t>(32), std::vector<uint8_t>(32), cb.callback());
  request->Start();
  request->AddDeviceForTesting(std::move(device0));
  request->AddDeviceForTesting(std::move(device1));
  std::tuple<U2fReturnCode, std::vector<uint8_t>, std::vector<uint8_t>>&
      response = cb.WaitForCallback();

  EXPECT_EQ(U2fReturnCode::SUCCESS, std::get<0>(response));
  ASSERT_GE(1u, std::get<1>(response).size());
  EXPECT_EQ(static_cast<uint8_t>(MockU2fDevice::kRegister),
            std::get<1>(response)[0]);

  // Verify that we get a blank key handle.
  EXPECT_TRUE(std::get<2>(response).empty());
}

}  // namespace device
