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

const std::string kTestErrorCodeTogglingIneligibleDevice =
    "Toggling ineligible device.";

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

  void SetUp() override {
    test_eligible_external_devices_infos_ = {
        available_external_device_infos_[0],
        available_external_device_infos_[2],
        available_external_device_infos_[3]};

    test_ineligible_external_devices_infos_ = {
        available_external_device_infos_[1],
        available_external_device_infos_[4]};

    CreateDeviceCapabilityManager();
  }

  void OnCryptAuthClientCreated(MockCryptAuthClient* client) override {
    ON_CALL(*client, ToggleEasyUnlock(_, _, _))
        .WillByDefault(
            Invoke(this, &DeviceCapabilityManagerTest::MockToggleEasyUnlock));
    ON_CALL(*client, FindEligibleUnlockDevices(_, _, _))
        .WillByDefault(Invoke(
            this, &DeviceCapabilityManagerTest::MockFindEligibleUnlockDevices));
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
    callback_toggle_device_ = callback;
  }

  void CreateDeviceCapabilityManager() {
    device_capability_manager_ = base::MakeUnique<DeviceCapabilityManager>(
        mock_cryptauth_client_factory_.get());
  }

  void CreateFindEligibleUnlockDevicesResponse() {
    find_eligible_unlock_devices_response_.Clear();
    for (unsigned long i = 0; i < test_eligible_external_devices_infos_.size();
         i++) {
      find_eligible_unlock_devices_response_.add_eligible_devices();
      *find_eligible_unlock_devices_response_.mutable_eligible_devices(i) =
          test_eligible_external_devices_infos_[i];
    }
    for (unsigned long i = 0;
         i < test_ineligible_external_devices_infos_.size(); i++) {
      find_eligible_unlock_devices_response_.add_ineligible_devices();
      find_eligible_unlock_devices_response_.mutable_ineligible_devices(i)
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

  void FindEligibleDevices() {
    device_capability_manager_->FindEligibleDevices(
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
    toggle_callback_called_ = true;
    callback_toggle_device_.Reset();
  }

  void TestSuccessCallbackFind(
      const std::vector<ExternalDeviceInfo>& eligible_devices,
      const std::vector<IneligibleDevice>& ineligible_devices) {
    find_callback_called_ = true;
    VerifyReturnedDevices(eligible_devices, ineligible_devices);
    callback_eligible_devices_.Reset();
  }

  void TestFailureCallback(const std::string& error_message) {
    EXPECT_EQ(error_message, kTestErrorCodeTogglingIneligibleDevice);
    failure_callback_called_ = true;
    callback_error_.Reset();
  }

  // At any given time, there should only be at most one callback that's valid.
  // Otherwise, device capability manager fails to gurantee order.  Pass in the
  // stored expected valid callback as such - !callback,is_null() - to
  // |valid_callback|.
  void InvokeCallbackPassedToCryptAuthClient(bool valid_callback) {
    EXPECT_TRUE(valid_callback);
    int number_of_valid_callbacks = !callback_toggle_device_.is_null() +
                                    !callback_eligible_devices_.is_null() +
                                    !callback_error_.is_null();
    ASSERT_EQ(number_of_valid_callbacks, 1);
    if (!callback_toggle_device_.is_null()) {
      callback_toggle_device_.Run(ToggleEasyUnlockResponse());
    } else if (!callback_eligible_devices_.is_null()) {
      CreateFindEligibleUnlockDevicesResponse();
      callback_eligible_devices_.Run(find_eligible_unlock_devices_response_);
    } else if (!callback_error_.is_null()) {
      EXPECT_NE(LastErrorMessage, "");
      callback_error_.Run(LastErrorMessage);
      LastErrorMessage = "";
    }
  }

  // Verify that the correct callback was executed by passing the associated
  // boolean that the associated callback flips on,  All other booleans should
  // be false as only one callback should have been executed.
  void VerifyCorrectCallbackCalledAndReset(bool callback_called) {
    EXPECT_TRUE(callback_called);
    int number_of_callbacks_called = toggle_callback_called_ +
                                     find_callback_called_ +
                                     failure_callback_called_;
    EXPECT_EQ(1, number_of_callbacks_called);
    toggle_callback_called_ = find_callback_called_ = failure_callback_called_ =
        false;
  }

  std::unique_ptr<MockCryptAuthClientFactory> mock_cryptauth_client_factory_;
  std::unique_ptr<cryptauth::DeviceCapabilityManager>
      device_capability_manager_;
  std::vector<cryptauth::RemoteDevice> available_devices_;
  std::vector<cryptauth::ExternalDeviceInfo> available_external_device_infos_;
  std::vector<ExternalDeviceInfo> test_eligible_external_devices_infos_;
  std::vector<ExternalDeviceInfo> test_ineligible_external_devices_infos_;
  FindEligibleUnlockDevicesResponse find_eligible_unlock_devices_response_;
  CryptAuthClient::ToggleEasyUnlockCallback callback_toggle_device_;
  CryptAuthClient::FindEligibleUnlockDevicesCallback callback_eligible_devices_;
  CryptAuthClient::ErrorCallback callback_error_;
  bool toggle_callback_called_ = false;
  bool find_callback_called_ = false;
  bool failure_callback_called_ = false;
  std::string LastErrorMessage;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceCapabilityManagerTest);
};

TEST_F(DeviceCapabilityManagerTest, TestOrderUponMultipleRequests) {
  // Request #1
  ToggleCapability(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                   test_eligible_external_devices_infos_[0], true /* enable */);
  EXPECT_FALSE(toggle_callback_called_);

  // Request #2
  FindEligibleDevices();
  EXPECT_FALSE(callback_eligible_devices_);

  // Request #3
  ToggleCapability(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                   test_eligible_external_devices_infos_[1], true /* enable */);
  EXPECT_FALSE(failure_callback_called_);

  // Request #4
  FindEligibleDevices();
  EXPECT_FALSE(callback_eligible_devices_);

  // Request #5
  ToggleCapability(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                   test_eligible_external_devices_infos_[2], true /* enable */);
  EXPECT_FALSE(toggle_callback_called_);

  // Request #6
  ToggleCapability(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                   test_ineligible_external_devices_infos_[0],
                   true /* enable */);
  EXPECT_FALSE(failure_callback_called_);

  InvokeCallbackPassedToCryptAuthClient(!callback_toggle_device_.is_null());
  VerifyCorrectCallbackCalledAndReset(toggle_callback_called_);

  InvokeCallbackPassedToCryptAuthClient(!callback_eligible_devices_.is_null());
  VerifyCorrectCallbackCalledAndReset(find_callback_called_);

  InvokeCallbackPassedToCryptAuthClient(!callback_toggle_device_.is_null());
  VerifyCorrectCallbackCalledAndReset(toggle_callback_called_);

  InvokeCallbackPassedToCryptAuthClient(!callback_eligible_devices_.is_null());
  VerifyCorrectCallbackCalledAndReset(find_callback_called_);

  InvokeCallbackPassedToCryptAuthClient(!callback_toggle_device_.is_null());
  VerifyCorrectCallbackCalledAndReset(toggle_callback_called_);

  InvokeCallbackPassedToCryptAuthClient(!callback_error_.is_null());
  VerifyCorrectCallbackCalledAndReset(failure_callback_called_);
}

TEST_F(DeviceCapabilityManagerTest, TestMultipleToggleCapabilityUnlockKeys) {
  ToggleCapability(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                   test_eligible_external_devices_infos_[0], true /* enable */);
  EXPECT_FALSE(toggle_callback_called_);

  ToggleCapability(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                   test_eligible_external_devices_infos_[1], true /* enable */);
  EXPECT_FALSE(toggle_callback_called_);

  ToggleCapability(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                   test_eligible_external_devices_infos_[2], true /* enable */);
  EXPECT_FALSE(toggle_callback_called_);

  InvokeCallbackPassedToCryptAuthClient(!callback_toggle_device_.is_null());
  VerifyCorrectCallbackCalledAndReset(toggle_callback_called_);

  InvokeCallbackPassedToCryptAuthClient(!callback_toggle_device_.is_null());
  VerifyCorrectCallbackCalledAndReset(toggle_callback_called_);

  InvokeCallbackPassedToCryptAuthClient(!callback_toggle_device_.is_null());
  VerifyCorrectCallbackCalledAndReset(toggle_callback_called_);
}

TEST_F(DeviceCapabilityManagerTest, TestMultipleFindEligibleDevices) {
  FindEligibleDevices();
  EXPECT_FALSE(find_callback_called_);

  FindEligibleDevices();
  EXPECT_FALSE(find_callback_called_);

  FindEligibleDevices();
  EXPECT_FALSE(find_callback_called_);

  InvokeCallbackPassedToCryptAuthClient(!callback_eligible_devices_.is_null());
  VerifyCorrectCallbackCalledAndReset(find_callback_called_);

  InvokeCallbackPassedToCryptAuthClient(!callback_eligible_devices_.is_null());
  VerifyCorrectCallbackCalledAndReset(find_callback_called_);

  InvokeCallbackPassedToCryptAuthClient(!callback_eligible_devices_.is_null());
  VerifyCorrectCallbackCalledAndReset(find_callback_called_);
}

}  // namespace cryptauth