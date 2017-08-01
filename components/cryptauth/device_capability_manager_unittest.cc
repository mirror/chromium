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
#include <unordered_set>
#include "base/bind.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "components/cryptauth/mock_cryptauth_client.h"
#include "components/cryptauth/remote_device.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;

namespace cryptauth {

namespace {

enum Actions {

};

const std::string kTestErrorCodeTogglingIneligibleDevice =
    "Toggling ineligible device.";

// Maybe move into remote_device_test_util?
std::vector<cryptauth::ExternalDeviceInfo>
CreateExternalDeviceInfosForRemoteDevices(
    const std::vector<cryptauth::RemoteDevice> remote_devices) {
  std::vector<cryptauth::ExternalDeviceInfo> device_infos;
  for (const auto& remote_device : remote_devices) {
    // Add an ExternalDeviceInfo with the same public key as the RemoteDevice.
    cryptauth::ExternalDeviceInfo info;
    info.set_public_key(remote_device.public_key);
    device_infos.push_back(info);
  }
  return device_infos;
}

std::unordered_set<std::string> GetPublicKeys(
    const std::vector<ExternalDeviceInfo>& external_device_info) {
  std::unordered_set<std::string> public_keys;
  for (const auto& device_info : external_device_info) {
    public_keys.insert(device_info.public_key());
  }
  return public_keys;
}

}  // namespace

class DeviceCapabilityManagerTest
    : public testing::Test,
      public MockCryptAuthClientFactory::Observer {
 public:
  DeviceCapabilityManagerTest()
      : mock_cryptauth_client_factory_(new MockCryptAuthClientFactory(
            MockCryptAuthClientFactory::MockType::MAKE_NICE_MOCKS)),
        available_devices_(cryptauth::GenerateTestRemoteDevices(5)),
        available_external_device_infos_(
            CreateExternalDeviceInfosForRemoteDevices(available_devices_)) {
    mock_cryptauth_client_factory_->AddObserver(this);
  }

  // This is the mock implementation of cryptauth client's
  // FindEligibleUnlcokDevices.
  void MockFindEligibleUnlockDevices(
      const FindEligibleUnlockDevicesRequest& request,
      const CryptAuthClient::FindEligibleUnlockDevicesCallback& callback,
      const CryptAuthClient::ErrorCallback& error_callback) {
    callback_eligible_devices_ = callback;
  }

  // This is the mock implementation of cryptauth client's ToggleEasyUnlock.
  void MockToggleEasyUnlock(
      const ToggleEasyUnlockRequest& request,
      const CryptAuthClient::ToggleEasyUnlockCallback& callback,
      const CryptAuthClient::ErrorCallback& error_callback) {
    std::unordered_set<std::string> eligible_public_keys =
        GetPublicKeys(test_eligible_external_devices_infos_);

    std::unordered_set<std::string> ineligible_public_keys =
        GetPublicKeys(test_ineligible_external_devices_infos_);

    if (eligible_public_keys.find(request.public_key()) ==
        eligible_public_keys.end()) {
      LastErrorMessage = kTestErrorCodeTogglingIneligibleDevice;
      callback_error_ = error_callback;
      return;
    }
    LOG(ERROR) << "SHOULD ONLY APPEAR ONCE.";
    callback_toggle_device_ = callback;
  }

  void OnCryptAuthClientCreated(MockCryptAuthClient* client) override {
    ON_CALL(*client, ToggleEasyUnlock(_, _, _))
        .WillByDefault(
            Invoke(this, &DeviceCapabilityManagerTest::MockToggleEasyUnlock));
    ON_CALL(*client, FindEligibleUnlockDevices(_, _, _))
        .WillByDefault(Invoke(
            this, &DeviceCapabilityManagerTest::MockFindEligibleUnlockDevices));
  }

  void SetUp() override {}

  void CreateDeviceCapabilityManager() {
    device_capability_manager_ = base::MakeUnique<DeviceCapabilityManager>(
        mock_cryptauth_client_factory_.get());
  }

  void FindEligibleDevices(DeviceCapabilityManager::Capability capability) {
    device_capability_manager_->FindEligibleDevices(
        capability,
        base::Bind(&DeviceCapabilityManagerTest::TestSuccessCallbackFind,
                   base::Unretained(this)),
        base::Bind(&DeviceCapabilityManagerTest::TestFailureCallback,
                   base::Unretained(this)));
  }

  void ToggleCapability(DeviceCapabilityManager::Capability capability,
                        const ExternalDeviceInfo& device_info,
                        bool enable) {
    device_capability_manager_->SetCapabilityForDevice(
        device_info.public_key(), capability, enable,
        base::Bind(&DeviceCapabilityManagerTest::TestSuccessCallbackToggle,
                   base::Unretained(this)),
        base::Bind(&DeviceCapabilityManagerTest::TestFailureCallback,
                   base::Unretained(this)));
  }

  void TestSuccessCallbackToggle() {
    LOG(ERROR) << "~~~ test success callback toggle called.";
    test_success_callback_toggle_called_ = true;
    callback_toggle_device_.Reset();
  }

  void TestSuccessCallbackFind(
      const std::vector<ExternalDeviceInfo>& eligible_devices,
      const std::vector<IneligibleDevice>& ineligible_devices) {
    test_success_callback_find_called_ = true;
    VerifyReturnedDevices(eligible_devices, ineligible_devices);
    callback_eligible_devices_.Reset();
  }

  void TestFailureCallback(const std::string& error_message) {
    EXPECT_EQ(error_message, kTestErrorCodeTogglingIneligibleDevice);
    callback_error_.Reset();
  }

  void CreateFindEligibleUnlockDevicesResponse() {
    find_eligible_unlock_devices_response.Clear();
    for (unsigned long i = 0; i < test_eligible_external_devices_infos_.size();
         i++) {
      find_eligible_unlock_devices_response.add_eligible_devices();
      *find_eligible_unlock_devices_response.mutable_eligible_devices(i) =
          test_eligible_external_devices_infos_[i];
    }
    for (unsigned long i = 0;
         i < test_ineligible_external_devices_infos_.size(); i++) {
      find_eligible_unlock_devices_response.add_ineligible_devices();
      find_eligible_unlock_devices_response.mutable_ineligible_devices(i)
          ->mutable_device()
          ->set_public_key(
              test_ineligible_external_devices_infos_[i].public_key());
    }
  }

  void VerifyReturnedDevices(
      const std::vector<ExternalDeviceInfo>& eligible_device_infos,
      const std::vector<IneligibleDevice>& ineligible_devices) {
    std::unordered_set<std::string> public_keys =
        GetPublicKeys(test_eligible_external_devices_infos_);
    for (const auto& remote_device : eligible_device_infos) {
      EXPECT_TRUE(public_keys.find(remote_device.public_key()) !=
                  public_keys.end());
    }
    public_keys = GetPublicKeys(test_ineligible_external_devices_infos_);
    for (const auto& ineligible_device : ineligible_devices) {
      EXPECT_TRUE(public_keys.find(ineligible_device.device().public_key()) !=
                  public_keys.end());
    }
  }

  void InvokeLastCallback(DeviceCapabilityManager::Capability capability,
                          bool successful) {
    if (!successful) {
      ASSERT_TRUE(!callback_error_.is_null());
      EXPECT_NE(LastErrorMessage, "");
      callback_error_.Run(LastErrorMessage);
      LastErrorMessage = "";
    } else if (capability ==
               DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY) {
      ASSERT_TRUE(!callback_toggle_device_.is_null());
      callback_toggle_device_.Run(ToggleEasyUnlockResponse());
    } else if (capability == DeviceCapabilityManager::Capability::
                                 CAPABILITY_READ_UNLOCK_DEVICES) {
      ASSERT_TRUE(!callback_eligible_devices_.is_null());
      CreateFindEligibleUnlockDevicesResponse();
      callback_eligible_devices_.Run(find_eligible_unlock_devices_response);
    }
  }

  void VerifyCorrectCallbackCalledAndReset(
      DeviceCapabilityManager::Capability capability,
      bool successful) {
    if (!successful) {
    } else if (capability ==
               DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY) {
      EXPECT_TRUE(test_success_callback_toggle_called_);
      EXPECT_FALSE(test_success_callback_find_called_);
    } else if (capability == DeviceCapabilityManager::Capability::
                                 CAPABILITY_READ_UNLOCK_DEVICES) {
      EXPECT_TRUE(test_success_callback_find_called_);
      EXPECT_FALSE(test_success_callback_toggle_called_);
    }
    test_success_callback_toggle_called_ = test_success_callback_find_called_ =
        false;
  }

  std::unique_ptr<MockCryptAuthClientFactory> mock_cryptauth_client_factory_;

  std::unique_ptr<cryptauth::DeviceCapabilityManager>
      device_capability_manager_;

  std::vector<cryptauth::RemoteDevice> available_devices_;
  std::vector<cryptauth::ExternalDeviceInfo> available_external_device_infos_;

  std::vector<ExternalDeviceInfo> test_eligible_external_devices_infos_;
  std::vector<ExternalDeviceInfo> test_ineligible_external_devices_infos_;

  FindEligibleUnlockDevicesResponse find_eligible_unlock_devices_response;

  CryptAuthClient::ToggleEasyUnlockCallback callback_toggle_device_;
  CryptAuthClient::FindEligibleUnlockDevicesCallback callback_eligible_devices_;
  CryptAuthClient::ErrorCallback callback_error_;
  bool test_success_callback_toggle_called_ = false;
  bool test_success_callback_find_called_ = false;
  std::string LastErrorMessage;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceCapabilityManagerTest);
};

TEST_F(DeviceCapabilityManagerTest, TestOrderUponMultipleRequests) {
  test_eligible_external_devices_infos_ = {
      available_external_device_infos_[0], available_external_device_infos_[1],
      available_external_device_infos_[2], available_external_device_infos_[3]};

  test_ineligible_external_devices_infos_ = {
      available_external_device_infos_[4]};

  CreateDeviceCapabilityManager();
  // Request #1
  ToggleCapability(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                   test_eligible_external_devices_infos_[0], true /* enable */);
  EXPECT_FALSE(test_success_callback_toggle_called_);

  // Request #2
  FindEligibleDevices();

  // Request #3
  ToggleCapability(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                   test_eligible_external_devices_infos_[1], true /* enable */);
  EXPECT_FALSE(test_success_callback_toggle_called_);

  // Request #4
  FindEligibleDevices();

  // Request #5
  ToggleCapability(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                   test_eligible_external_devices_infos_[2], true /* enable */);
  EXPECT_FALSE(test_success_callback_toggle_called_);

  // Request #6
  ToggleCapability(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                   test_ineligible_external_devices_infos_[0],
                   true /* enable */);

  InvokeLastCallback(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                     true /*successful*/);
  VerifyCorrectCallbackCalledAndReset(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
      true /*successful*/);

  InvokeLastCallback(
      DeviceCapabilityManager::Capability::CAPABILITY_READ_UNLOCK_DEVICES,
      true /*successful*/);
  VerifyCorrectCallbackCalledAndReset(
      DeviceCapabilityManager::Capability::CAPABILITY_READ_UNLOCK_DEVICES,
      true /*successful*/);

  InvokeLastCallback(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                     true /*successful*/);
  VerifyCorrectCallbackCalledAndReset(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
      true /*successful*/);

  InvokeLastCallback(
      DeviceCapabilityManager::Capability::CAPABILITY_READ_UNLOCK_DEVICES,
      true /*successful*/);
  VerifyCorrectCallbackCalledAndReset(
      DeviceCapabilityManager::Capability::CAPABILITY_READ_UNLOCK_DEVICES,
      true /*successful*/);

  InvokeLastCallback(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                     true /*successful*/);
  VerifyCorrectCallbackCalledAndReset(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
      true /*successful*/);

  InvokeLastCallback(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                     false /*successful*/);
  VerifyCorrectCallbackCalledAndReset(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
      false /*successful*/);
}

}  // namespace cryptauth