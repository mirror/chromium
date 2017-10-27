// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <list>
#include <utility>

#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "device/u2f/mock_u2f_device.h"
#include "device/u2f/mock_u2f_discovery.h"
#include "device/u2f/u2f_register.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace device {

class U2fRegisterTest : public testing::Test {
 public:
  U2fRegisterTest()
      : scoped_task_environment_(
            base::test::ScopedTaskEnvironment::MainThreadType::UI) {}

 protected:
  base::test::ScopedTaskEnvironment scoped_task_environment_;
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

TEST_F(U2fRegisterTest, TestRegisterSuccess) {
  std::unique_ptr<MockU2fDevice> device(new MockU2fDevice());
  auto discovery = std::make_unique<MockU2fDiscovery>();

  EXPECT_CALL(*device.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));
  EXPECT_CALL(*device.get(), DeviceTransactPtr(testing::_, testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));
  EXPECT_CALL(*device.get(), TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(*discovery, Start())
      .WillOnce(testing::Invoke(discovery.get(),
                                &MockU2fDiscovery::StartSuccessAsync));

  TestRegisterCallback cb;
  std::vector<std::unique_ptr<U2fDiscovery>> discoveries;
  MockU2fDiscovery* discovery_weak = discovery.get();
  discoveries.push_back(std::move(discovery));
  std::vector<std::vector<uint8_t>> registration_keys;
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      registration_keys, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      std::move(discoveries), cb.callback());
  request->Start();
  discovery_weak->AddDevice(std::move(device));
  std::pair<U2fReturnCode, std::vector<uint8_t>>& response =
      cb.WaitForCallback();
  EXPECT_EQ(U2fReturnCode::SUCCESS, response.first);
  ASSERT_LT(static_cast<size_t>(0), response.second.size());
  EXPECT_EQ(static_cast<uint8_t>(MockU2fDevice::kRegister), response.second[0]);
}

TEST_F(U2fRegisterTest, TestDelayedSuccess) {
  std::unique_ptr<MockU2fDevice> device(new MockU2fDevice());
  auto discovery = std::make_unique<MockU2fDiscovery>();

  EXPECT_CALL(*device.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));
  // Go through the state machine twice before success
  EXPECT_CALL(*device.get(), DeviceTransactPtr(testing::_, testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::NotSatisfied))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));
  EXPECT_CALL(*device.get(), TryWink(testing::_))
      .Times(2)
      .WillRepeatedly(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(*discovery, Start())
      .WillOnce(testing::Invoke(discovery.get(),
                                &MockU2fDiscovery::StartSuccessAsync));
  TestRegisterCallback cb;

  std::vector<std::unique_ptr<U2fDiscovery>> discoveries;
  MockU2fDiscovery* discovery_weak = discovery.get();
  std::vector<std::vector<uint8_t>> registration_keys;
  discoveries.push_back(std::move(discovery));
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      registration_keys, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      std::move(discoveries), cb.callback());
  request->Start();
  discovery_weak->AddDevice(std::move(device));
  std::pair<U2fReturnCode, std::vector<uint8_t>>& response =
      cb.WaitForCallback();
  EXPECT_EQ(U2fReturnCode::SUCCESS, response.first);
  ASSERT_LT(static_cast<size_t>(0), response.second.size());
  EXPECT_EQ(static_cast<uint8_t>(MockU2fDevice::kRegister), response.second[0]);
}

TEST_F(U2fRegisterTest, TestMultipleDevices) {
  // Second device will have a successful touch
  std::unique_ptr<MockU2fDevice> device0(new MockU2fDevice());
  std::unique_ptr<MockU2fDevice> device1(new MockU2fDevice());
  auto discovery = std::make_unique<MockU2fDiscovery>();

  EXPECT_CALL(*device0.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));
  EXPECT_CALL(*device1.get(), GetId())
      .WillRepeatedly(testing::Return("device1"));
  EXPECT_CALL(*device0.get(), DeviceTransactPtr(testing::_, testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::NotSatisfied));
  // One wink per device
  EXPECT_CALL(*device0.get(), TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(*device1.get(), DeviceTransactPtr(testing::_, testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));
  EXPECT_CALL(*device1.get(), TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(*discovery, Start())
      .WillOnce(testing::Invoke(discovery.get(),
                                &MockU2fDiscovery::StartSuccessAsync));

  TestRegisterCallback cb;
  std::vector<std::unique_ptr<U2fDiscovery>> discoveries;
  MockU2fDiscovery* discovery_weak = discovery.get();
  discoveries.push_back(std::move(discovery));
  std::vector<std::vector<uint8_t>> registration_keys;
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      registration_keys, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      std::move(discoveries), cb.callback());
  request->Start();
  discovery_weak->AddDevice(std::move(device0));
  discovery_weak->AddDevice(std::move(device1));
  std::pair<U2fReturnCode, std::vector<uint8_t>>& response =
      cb.WaitForCallback();
  EXPECT_EQ(U2fReturnCode::SUCCESS, response.first);
  ASSERT_LT(static_cast<size_t>(0), response.second.size());
  EXPECT_EQ(static_cast<uint8_t>(MockU2fDevice::kRegister), response.second[0]);
}

TEST_F(U2fRegisterTest, TestSingleDeviceRegistrationWithEmptyExclusionList) {
  std::vector<std::vector<uint8_t>> empty_registered_keys;

  std::unique_ptr<MockU2fDevice> device(new MockU2fDevice());
  auto discovery = std::make_unique<MockU2fDiscovery>();
  EXPECT_CALL(*device.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));
  EXPECT_CALL(*device.get(), DeviceTransactPtr(testing::_, testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));
  EXPECT_CALL(*device.get(), TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(*discovery, Start())
      .WillOnce(testing::Invoke(discovery.get(),
                                &MockU2fDiscovery::StartSuccessAsync));

  TestRegisterCallback cb;
  std::vector<std::unique_ptr<U2fDiscovery>> discoveries;
  MockU2fDiscovery* discovery_weak = discovery.get();
  discoveries.push_back(std::move(discovery));
  std::vector<std::vector<uint8_t>> registration_keys;
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      empty_registered_keys, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      std::move(discoveries), cb.callback());
  request->Start();

  discovery_weak->AddDevice(std::move(device));
  std::pair<U2fReturnCode, std::vector<uint8_t>>& response =
      cb.WaitForCallback();
  EXPECT_EQ(U2fReturnCode::SUCCESS, response.first);
  ASSERT_LT(static_cast<size_t>(0), response.second.size());
  EXPECT_EQ(static_cast<uint8_t>(MockU2fDevice::kRegister), response.second[0]);
}

TEST_F(U2fRegisterTest, TestSingleDeviceRegistrationWithExclusionList) {
  std::vector<uint8_t> unknown_key0(32, 0xB);
  std::vector<uint8_t> unknown_key1(32, 0xC);
  std::vector<uint8_t> unknown_key2(32, 0xD);
  // Put wrong key first to ensure that it will be tested before the correct key
  std::vector<std::vector<uint8_t>> handles = {unknown_key0, unknown_key1,
                                               unknown_key2};
  std::unique_ptr<MockU2fDevice> device(new MockU2fDevice());
  auto discovery = std::make_unique<MockU2fDiscovery>();

  EXPECT_CALL(*device.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));
  // Only one wink expected per device
  EXPECT_CALL(*device.get(), DeviceTransactPtr(testing::_, testing::_))
      .Times(4)
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));
  EXPECT_CALL(*device.get(), TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(*discovery, Start())
      .WillOnce(testing::Invoke(discovery.get(),
                                &MockU2fDiscovery::StartSuccessAsync));

  TestRegisterCallback cb;
  std::vector<std::unique_ptr<U2fDiscovery>> discoveries;
  MockU2fDiscovery* discovery_weak = discovery.get();
  discoveries.push_back(std::move(discovery));
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      handles, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      std::move(discoveries), cb.callback());
  discovery_weak->AddDevice(std::move(device));
  std::pair<U2fReturnCode, std::vector<uint8_t>>& response =
      cb.WaitForCallback();
  EXPECT_EQ(U2fReturnCode::SUCCESS, response.first);
  ASSERT_LT(static_cast<size_t>(0), response.second.size());
  EXPECT_EQ(static_cast<uint8_t>(MockU2fDevice::kRegister), response.second[0]);
}

TEST_F(U2fRegisterTest, TestMultipleDeviceRegistrationWithExclusionList) {
  std::vector<uint8_t> unknown_key0(32, 0xB);
  std::vector<uint8_t> unknown_key1(32, 0xC);
  std::vector<uint8_t> unknown_key2(32, 0xD);
  // Put wrong key first to ensure that it will be tested before the correct key
  std::vector<std::vector<uint8_t>> handles = {unknown_key0, unknown_key1,
                                               unknown_key2};
  std::unique_ptr<MockU2fDevice> device0(new MockU2fDevice());
  std::unique_ptr<MockU2fDevice> device1(new MockU2fDevice());
  auto discovery = std::make_unique<MockU2fDiscovery>();

  EXPECT_CALL(*device0.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));
  EXPECT_CALL(*device1.get(), GetId())
      .WillRepeatedly(testing::Return("device1"));

  // Only one wink expected per device
  EXPECT_CALL(*device0.get(), DeviceTransactPtr(testing::_, testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::NotSatisfied));
  EXPECT_CALL(*device1.get(), DeviceTransactPtr(testing::_, testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));

  EXPECT_CALL(*device0.get(), TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));
  EXPECT_CALL(*device1.get(), TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));

  EXPECT_CALL(*discovery, Start())
      .WillOnce(testing::Invoke(discovery.get(),
                                &MockU2fDiscovery::StartSuccessAsync));

  TestRegisterCallback cb;
  std::vector<std::unique_ptr<U2fDiscovery>> discoveries;
  MockU2fDiscovery* discovery_weak = discovery.get();
  discoveries.push_back(std::move(discovery));
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      handles, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      std::move(discoveries), cb.callback());

  discovery_weak->AddDevice(std::move(device0));
  discovery_weak->AddDevice(std::move(device1));
  std::pair<U2fReturnCode, std::vector<uint8_t>>& response =
      cb.WaitForCallback();

  EXPECT_EQ(U2fReturnCode::SUCCESS, response.first);
  ASSERT_LT(static_cast<size_t>(0), response.second.size());
  EXPECT_EQ(static_cast<uint8_t>(MockU2fDevice::kRegister), response.second[0]);
}

TEST_F(U2fRegisterTest, TestSingleDeviceRegistrationWithDuplicateHandle) {
  std::vector<uint8_t> duplicate_key(32, 0xA);
  std::vector<uint8_t> wrong_key0(32, 0xB);
  std::vector<uint8_t> wrong_key1(32, 0xC);
  std::vector<uint8_t> wrong_key2(32, 0xD);
  // Put wrong key first to ensure that it will be tested before the correct key
  std::vector<std::vector<uint8_t>> handles = {wrong_key0, wrong_key1,
                                               wrong_key2, duplicate_key};
  std::unique_ptr<MockU2fDevice> device(new MockU2fDevice());
  auto discovery = std::make_unique<MockU2fDiscovery>();

  EXPECT_CALL(*device.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));

  // Only one wink expected per device
  EXPECT_CALL(*device.get(), DeviceTransactPtr(testing::_, testing::_))
      .Times(5)
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorSign))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));
  EXPECT_CALL(*device.get(), TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));

  EXPECT_CALL(*discovery, Start())
      .WillOnce(testing::Invoke(discovery.get(),
                                &MockU2fDiscovery::StartSuccessAsync));
  TestRegisterCallback cb;
  std::vector<std::unique_ptr<U2fDiscovery>> discoveries;
  MockU2fDiscovery* discovery_weak = discovery.get();
  discoveries.push_back(std::move(discovery));
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      handles, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      std::move(discoveries), cb.callback());
  discovery_weak->AddDevice(std::move(device));
  std::pair<U2fReturnCode, std::vector<uint8_t>>& response =
      cb.WaitForCallback();
  EXPECT_EQ(U2fReturnCode::CONDITIONS_NOT_SATISFIED, response.first);
}

TEST_F(U2fRegisterTest, TestMultipleDeviceRegistrationWithDuplicateHandle) {
  std::vector<uint8_t> duplicate_key(32, 0xA);
  std::vector<uint8_t> wrong_key0(32, 0xB);
  std::vector<uint8_t> wrong_key1(32, 0xC);
  std::vector<uint8_t> wrong_key2(32, 0xD);

  // Put wrong key first to ensure that it will be tested before the correct key
  std::vector<std::vector<uint8_t>> handles = {wrong_key0, wrong_key1,
                                               wrong_key2, duplicate_key};
  std::unique_ptr<MockU2fDevice> device0(new MockU2fDevice());
  std::unique_ptr<MockU2fDevice> device1(new MockU2fDevice());
  auto discovery = std::make_unique<MockU2fDiscovery>();

  EXPECT_CALL(*device0.get(), GetId())
      .WillRepeatedly(testing::Return("device0"));

  EXPECT_CALL(*device1.get(), GetId())
      .WillRepeatedly(testing::Return("device1"));

  EXPECT_CALL(*device0.get(), DeviceTransactPtr(testing::_, testing::_))
      .Times(4)
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData));
  EXPECT_CALL(*device1.get(), DeviceTransactPtr(testing::_, testing::_))
      .Times(5)
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::WrongData))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorSign))
      .WillOnce(testing::Invoke(MockU2fDevice::NoErrorRegister));

  EXPECT_CALL(*device0.get(), TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));

  EXPECT_CALL(*device1.get(), TryWink(testing::_))
      .WillOnce(testing::Invoke(MockU2fDevice::WinkDoNothing));

  EXPECT_CALL(*discovery, Start())
      .WillOnce(testing::Invoke(discovery.get(),
                                &MockU2fDiscovery::StartSuccessAsync));
  TestRegisterCallback cb;
  std::vector<std::unique_ptr<U2fDiscovery>> discoveries;
  MockU2fDiscovery* discovery_weak = discovery.get();
  discoveries.push_back(std::move(discovery));
  std::unique_ptr<U2fRequest> request = U2fRegister::TryRegistration(
      handles, std::vector<uint8_t>(32), std::vector<uint8_t>(32),
      std::move(discoveries), cb.callback());
  discovery_weak->AddDevice(std::move(device0));
  discovery_weak->AddDevice(std::move(device1));

  std::pair<U2fReturnCode, std::vector<uint8_t>>& response =
      cb.WaitForCallback();
  EXPECT_EQ(U2fReturnCode::CONDITIONS_NOT_SATISFIED, response.first);
}

}  // namespace device
