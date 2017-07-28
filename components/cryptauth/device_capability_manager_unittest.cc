// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/device_capability_manager.h"
#include <stddef.h>
#include <memory>
#include <utility>
#include "base/logging.h"

#include <chrono>
#include <thread>
#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "components/cryptauth/mock_cryptauth_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;

namespace cryptauth {

class DeviceCapabilityManagerTest
    : public testing::Test,
      public MockCryptAuthClientFactory::Observer {
 public:
  DeviceCapabilityManagerTest()
      : mock_cryptauth_client_factory_(new MockCryptAuthClientFactory(
            MockCryptAuthClientFactory::MockType::MAKE_NICE_MOCKS)) {
    mock_cryptauth_client_factory_->AddObserver(this);
  }

  void FindEligibleUnlockDevicesTest(
      const FindEligibleUnlockDevicesRequest& request,
      const CryptAuthClient::FindEligibleUnlockDevicesCallback& callback,
      const CryptAuthClient::ErrorCallback& error_callback) {
    LOG(ERROR) << "FOR REGAN - Cryptauth_client overrided func called ";
    FindEligibleUnlockDevicesResponse ugh = FindEligibleUnlockDevicesResponse();
    callback.Run(ugh);
  }

  void ToggleEasyUnlockTest(
      const ToggleEasyUnlockRequest& request,
      const CryptAuthClient::ToggleEasyUnlockCallback& callback,
      const CryptAuthClient::ErrorCallback& error_callback) {
    callback.Run(ToggleEasyUnlockResponse());
  }

  void OnCryptAuthClientCreated(MockCryptAuthClient* client) override {
    ON_CALL(*client, ToggleEasyUnlock(_, _, _))
        .WillByDefault(
            Invoke(this, &DeviceCapabilityManagerTest::ToggleEasyUnlockTest));
    ON_CALL(*client, FindEligibleUnlockDevices(_, _, _))
        .WillByDefault(Invoke(
            this, &DeviceCapabilityManagerTest::FindEligibleUnlockDevicesTest));
  }

  void SetUp() override {}

  void CreateDeviceCapabilityManager() {
    device_capability_manager_ = base::MakeUnique<DeviceCapabilityManager>(
        mock_cryptauth_client_factory_.get());
  }

  void FindEligibleDevices(DeviceCapabilityManager::Capability capability) {
    device_capability_manager_->FindEligibleDevices(
        capability,
        base::Bind(&DeviceCapabilityManagerTest::TestSuccessCallback,
                   base::Unretained(this)),
        base::Bind(&DeviceCapabilityManagerTest::TestFailureCallback,
                   base::Unretained(this)));
  }

  void TestSuccessCallback(const std::vector<ExternalDeviceInfo>& devices) {
    LOG(ERROR) << "FOR REGAN- caller's func(success) - ";
  }

  void TestFailureCallback(const std::string& error_message) {
    LOG(ERROR) << "FOR REGAN- caller's func(error) - " << error_message;
  }

  std::unique_ptr<MockCryptAuthClientFactory> mock_cryptauth_client_factory_;

  std::unique_ptr<cryptauth::DeviceCapabilityManager>
      device_capability_manager_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceCapabilityManagerTest);
};

TEST_F(DeviceCapabilityManagerTest, TestMultipleSetCapabilities) {
  CreateDeviceCapabilityManager();
  FindEligibleDevices(
      DeviceCapabilityManager::Capability::CAPABILITY_READ_UNLOCK_DEVICES);
  device_capability_manager_->FindEligibleDevices(
      DeviceCapabilityManager::Capability::CAPABILITY_READ_UNLOCK_DEVICES,
      base::Bind(&DeviceCapabilityManagerTest::TestSuccessCallback,
                 base::Unretained(this)),
      base::Bind(&DeviceCapabilityManagerTest::TestFailureCallback,
                 base::Unretained(this)));
  EXPECT_EQ(0, 0);
}

}  // namespace cryptauth