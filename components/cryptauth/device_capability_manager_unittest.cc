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

class DeviceCapabilityManagerTest : public testing::Test {
 public:
  class TestCryptAuthClient : public MockCryptAuthClient {
   public:
    TestCryptAuthClient() : MockCryptAuthClient(){};
    void ToggleEasyUnlock(const ToggleEasyUnlockRequest& request,
                          const ToggleEasyUnlockCallback& callback,
                          const ErrorCallback& error_callback) override {
      LOG(ERROR) << "FOR REGAN - Cryptauth_client overrided func called "
                    "ToggleEasyUnlock";
    }
    void FindEligibleUnlockDevices(
        const FindEligibleUnlockDevicesRequest& request,
        const FindEligibleUnlockDevicesCallback& callback,
        const ErrorCallback& error_callback) override {
      LOG(ERROR) << "FOR REGAN - Cryptauth_client overrided func called "
                    "FindEligibleUnlockDevices";
      FindEligibleUnlockDevicesResponse ugh =
          FindEligibleUnlockDevicesResponse();
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      callback.Run(ugh);
    }
  };
  class TestCryptAuthClientFactory : public CryptAuthClientFactory {
   public:
    TestCryptAuthClientFactory(){};
    std::unique_ptr<CryptAuthClient> CreateInstance() override {
      std::unique_ptr<TestCryptAuthClient> client;
      client.reset(new testing::NiceMock<TestCryptAuthClient>());
      return std::move(client);
    }
  };

  DeviceCapabilityManagerTest() {}

  void SetUp() override {
    mock_cryptauth_client_factory_ =
        base::MakeUnique<TestCryptAuthClientFactory>();
  }

  void CreateDeviceCapabilityManager() {
    device_capability_manager_ = base::MakeUnique<DeviceCapabilityManager>(
        mock_cryptauth_client_factory_.get());
  }

  void TestSuccessCallback(const std::vector<ExternalDeviceInfo>& devices) {
    LOG(ERROR) << "FOR REGAN- caller's func(success) - ";
  }

  void TestFailureCallback(const std::string& error_message) {
    LOG(ERROR) << "FOR REGAN- caller's func(error) - " << error_message;
  }

  std::unique_ptr<TestCryptAuthClientFactory> mock_cryptauth_client_factory_;

  std::unique_ptr<cryptauth::DeviceCapabilityManager>
      device_capability_manager_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceCapabilityManagerTest);
};

TEST_F(DeviceCapabilityManagerTest, TestMultipleSetCapabilities) {
  CreateDeviceCapabilityManager();
  device_capability_manager_->FindEligibleDevices(
      DeviceCapabilityManager::Capability::CAPABILITY_READ_UNLOCK_DEVICES,
      base::Bind(&DeviceCapabilityManagerTest::TestSuccessCallback,
                 base::Unretained(this)),
      base::Bind(&DeviceCapabilityManagerTest::TestFailureCallback,
                 base::Unretained(this)));
  device_capability_manager_->FindEligibleDevices(
      DeviceCapabilityManager::Capability::CAPABILITY_READ_UNLOCK_DEVICES,
      base::Bind(&DeviceCapabilityManagerTest::TestSuccessCallback,
                 base::Unretained(this)),
      base::Bind(&DeviceCapabilityManagerTest::TestFailureCallback,
                 base::Unretained(this)));
  EXPECT_EQ(0, 0);
}

}  // namespace cryptauth