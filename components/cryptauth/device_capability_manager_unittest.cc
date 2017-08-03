// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/device_capability_manager.h"

#include <stddef.h>

#include <chrono>
#include <memory>
#include <thread>
#include <unordered_set>
#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "components/cryptauth/mock_cryptauth_client.h"
#include "components/cryptauth/remote_device.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Invoke;

namespace cryptauth {

namespace {

const char kSuccessResult[] = "success";
const char kErrorOnToggleEasyUnlockResult[] = "toggle easy unlock error";
const char kErrorFindEligibleUnlockDevices[] =
    "find eligible unlock device error";
const char kErrorFindEligibleForPromotion[] =
    "find eligible for promotion error";
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
      : available_devices_(cryptauth::GenerateTestRemoteDevices(5)),
        available_external_device_infos_(
            CreateExternalDeviceInfosForRemoteDevices(available_devices_)),
        test_eligible_external_devices_infos_(
            {available_external_device_infos_[0],
             available_external_device_infos_[1],
             available_external_device_infos_[2]}),
        test_ineligible_external_devices_infos_(
            {available_external_device_infos_[3],
             available_external_device_infos_[4]}) {}

  void SetUp() override {
    mock_cryptauth_client_factory_ =
        base::MakeUnique<MockCryptAuthClientFactory>(
            MockCryptAuthClientFactory::MockType::MAKE_NICE_MOCKS);
    mock_cryptauth_client_factory_->AddObserver(this);
    device_capability_manager_ = base::MakeUnique<DeviceCapabilityManager>(
        mock_cryptauth_client_factory_.get());
  }

  void OnCryptAuthClientCreated(MockCryptAuthClient* client) override {
    ON_CALL(*client, ToggleEasyUnlock(_, _, _))
        .WillByDefault(
            Invoke(this, &DeviceCapabilityManagerTest::MockToggleEasyUnlock));
    ON_CALL(*client, FindEligibleUnlockDevices(_, _, _))
        .WillByDefault(Invoke(
            this, &DeviceCapabilityManagerTest::MockFindEligibleUnlockDevices));
    ON_CALL(*client, FindEligibleForPromotion(_, _, _))
        .WillByDefault(Invoke(
            this, &DeviceCapabilityManagerTest::MockFindEligibleForPromotion));
  }

  // Mock CryptAuthClient::ToggleEasyUnlock() implementation.
  void MockToggleEasyUnlock(
      const ToggleEasyUnlockRequest& request,
      const CryptAuthClient::ToggleEasyUnlockCallback& callback,
      const CryptAuthClient::ErrorCallback& error_callback) {
    toggle_easy_unlock_callback_ = callback;
    error_callback_ = error_callback;
    error_code_ = kErrorOnToggleEasyUnlockResult;
  }

  // Mock CryptAuthClient::FindEligibleUnlockDevices() implementation.
  void MockFindEligibleUnlockDevices(
      const FindEligibleUnlockDevicesRequest& request,
      const CryptAuthClient::FindEligibleUnlockDevicesCallback& callback,
      const CryptAuthClient::ErrorCallback& error_callback) {
    find_eligible_unlock_devices_callback_ = callback;
    error_callback_ = error_callback;
    error_code_ = kErrorFindEligibleUnlockDevices;
  }

  // Mock CryptAuthClient::FindEligibleForPromotion() implementation.
  void MockFindEligibleForPromotion(
      const FindEligibleForPromotionRequest& request,
      const CryptAuthClient::FindEligibleForPromotionCallback& callback,
      const CryptAuthClient::ErrorCallback& error_callback) {
    find_eligible_for_promotion_callback_ = callback;
    error_callback_ = error_callback;
    error_code_ = kErrorFindEligibleForPromotion;
  }

  void CreateFindEligibleUnlockDevicesResponse() {
    find_eligible_unlock_devices_response_.Clear();
    for (const auto& device_info : test_eligible_external_devices_infos_) {
      find_eligible_unlock_devices_response_.add_eligible_devices()->CopyFrom(
          device_info);
    }
    for (const auto& device_info : test_ineligible_external_devices_infos_) {
      find_eligible_unlock_devices_response_.add_ineligible_devices()
          ->mutable_device()
          ->CopyFrom(device_info);
    }
  }

  void VerifyReturnedDevices(
      std::vector<ExternalDeviceInfo>& result_eligible_device_infos,
      std::vector<IneligibleDevice>& result_ineligible_devices) {
    std::unordered_set<std::string> public_keys =
        GetPublicKeys(test_eligible_external_devices_infos_);
    for (const auto& remote_device : result_eligible_device_infos) {
      EXPECT_TRUE(public_keys.find(remote_device.public_key()) !=
                  public_keys.end());
    }
    public_keys = GetPublicKeys(test_ineligible_external_devices_infos_);
    for (const auto& ineligible_device : result_ineligible_devices) {
      EXPECT_TRUE(public_keys.find(ineligible_device.device().public_key()) !=
                  public_keys.end());
    }
    result_eligible_device_infos.clear();
    result_ineligible_devices.clear();
  }

  void SetCapabilityForDevice(DeviceCapabilityManager::Capability capability,
                              const ExternalDeviceInfo& device_info,
                              bool enable) {
    device_capability_manager_->SetCapabilityForDevice(
        device_info.public_key(), capability, enable,
        base::Bind(&DeviceCapabilityManagerTest::
                       TestSuccessSetCapabilityKeyUnlockCallback,
                   base::Unretained(this)),
        base::Bind(&DeviceCapabilityManagerTest::TestErrorCallback,
                   base::Unretained(this)));
  }

  void FindEligibleDevices(DeviceCapabilityManager::Capability capability) {
    device_capability_manager_->FindEligibleDevices(
        DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
        base::Bind(&DeviceCapabilityManagerTest::
                       TestSuccessFindEligibleUnlockDevicesCallback,
                   base::Unretained(this)),
        base::Bind(&DeviceCapabilityManagerTest::TestErrorCallback,
                   base::Unretained(this)));
  }

  void IsPromotableDevice(DeviceCapabilityManager::Capability capability,
                          std::string public_key) {
    device_capability_manager_->IsDevicePromotable(
        public_key, capability,
        base::Bind(&DeviceCapabilityManagerTest::
                       TestSuccessFindEligibleForPromotionDeviceCallback,
                   base::Unretained(this)),
        base::Bind(&DeviceCapabilityManagerTest::TestErrorCallback,
                   base::Unretained(this)));
  }

  void TestSuccessSetCapabilityKeyUnlockCallback() { result_ = kSuccessResult; }

  void TestSuccessFindEligibleUnlockDevicesCallback(
      const std::vector<ExternalDeviceInfo>& eligible_devices,
      const std::vector<IneligibleDevice>& ineligible_devices) {
    result_ = kSuccessResult;
    result_eligible_devices_ = eligible_devices;
    result_ineligible_devices_ = ineligible_devices;
  }

  void TestSuccessFindEligibleForPromotionDeviceCallback(bool eligible) {
    result_ = kSuccessResult;
    result_eligible_for_promotion_ = eligible;
  }

  void TestErrorCallback(const std::string& error_message) {
    result_ = error_message;
  }

  void InvokeSetCapabilityKeyUnlockCallback() {
    CryptAuthClient::ToggleEasyUnlockCallback success_callback =
        toggle_easy_unlock_callback_;
    ASSERT_TRUE(!success_callback.is_null());
    toggle_easy_unlock_callback_.Reset();
    success_callback.Run(ToggleEasyUnlockResponse());
  }

  void InvokeFindEligibleUnlockDevicesCallback() {
    CreateFindEligibleUnlockDevicesResponse();
    CryptAuthClient::FindEligibleUnlockDevicesCallback success_callback =
        find_eligible_unlock_devices_callback_;
    ASSERT_TRUE(!success_callback.is_null());
    find_eligible_unlock_devices_callback_.Reset();
    success_callback.Run(find_eligible_unlock_devices_response_);
  }

  void InvokeFindEligibleForPromotionCallback(bool eligible_for_promotion) {
    FindEligibleForPromotionResponse response;
    response.set_may_show_promo(eligible_for_promotion);
    CryptAuthClient::FindEligibleForPromotionCallback success_callback =
        find_eligible_for_promotion_callback_;
    ASSERT_TRUE(!success_callback.is_null());
    find_eligible_for_promotion_callback_.Reset();
    success_callback.Run(response);
  }

  void InvokeErrorCallback() {
    CryptAuthClient::ErrorCallback error_callback = error_callback_;
    ASSERT_TRUE(!error_callback.is_null());
    error_callback_.Reset();
    error_callback.Run(error_code_);
  }

  std::string GetResultAndReset() {
    std::string result;
    result.swap(result_);
    return result;
  }

  bool GetEligibleForPromotionAndReset() {
    bool result_eligible_for_promotion = result_eligible_for_promotion_;
    result_eligible_for_promotion_ = !result_eligible_for_promotion;
    return result_eligible_for_promotion;
  }

  const std::vector<cryptauth::RemoteDevice> available_devices_;
  const std::vector<cryptauth::ExternalDeviceInfo>
      available_external_device_infos_;
  const std::vector<ExternalDeviceInfo> test_eligible_external_devices_infos_;
  const std::vector<ExternalDeviceInfo> test_ineligible_external_devices_infos_;

  std::unique_ptr<MockCryptAuthClientFactory> mock_cryptauth_client_factory_;
  std::unique_ptr<cryptauth::DeviceCapabilityManager>
      device_capability_manager_;
  FindEligibleUnlockDevicesResponse find_eligible_unlock_devices_response_;
  CryptAuthClient::ToggleEasyUnlockCallback toggle_easy_unlock_callback_;
  CryptAuthClient::FindEligibleUnlockDevicesCallback
      find_eligible_unlock_devices_callback_;
  CryptAuthClient::FindEligibleForPromotionCallback
      find_eligible_for_promotion_callback_;
  CryptAuthClient::ErrorCallback error_callback_;

  // For all tests.
  std::string result_;
  std::string error_code_;
  // For FindEligibleUnlockDevice tests.
  std::vector<ExternalDeviceInfo> result_eligible_devices_;
  std::vector<IneligibleDevice> result_ineligible_devices_;
  // For FindEligibleForPromotion tests.
  bool result_eligible_for_promotion_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DeviceCapabilityManagerTest);
};

TEST_F(DeviceCapabilityManagerTest, TestOrderUponMultipleRequests) {
  SetCapabilityForDevice(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
      test_eligible_external_devices_infos_[0], true /* enable */);
  FindEligibleDevices(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY);
  IsPromotableDevice(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                     test_eligible_external_devices_infos_[0].public_key());
  SetCapabilityForDevice(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
      test_eligible_external_devices_infos_[1], true /* enable */);
  FindEligibleDevices(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY);
  IsPromotableDevice(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                     test_eligible_external_devices_infos_[1].public_key());

  InvokeSetCapabilityKeyUnlockCallback();
  EXPECT_EQ(kSuccessResult, GetResultAndReset());

  InvokeFindEligibleUnlockDevicesCallback();
  EXPECT_EQ(kSuccessResult, GetResultAndReset());
  VerifyReturnedDevices(result_eligible_devices_, result_ineligible_devices_);

  InvokeFindEligibleForPromotionCallback(true /*eligible*/);
  EXPECT_TRUE(GetEligibleForPromotionAndReset());

  InvokeSetCapabilityKeyUnlockCallback();
  EXPECT_EQ(kSuccessResult, GetResultAndReset());

  InvokeFindEligibleUnlockDevicesCallback();
  EXPECT_EQ(kSuccessResult, GetResultAndReset());
  VerifyReturnedDevices(result_eligible_devices_, result_ineligible_devices_);

  InvokeFindEligibleForPromotionCallback(true /*eligible*/);
  EXPECT_TRUE(GetEligibleForPromotionAndReset());
}

TEST_F(DeviceCapabilityManagerTest, TestMultipleSetUnlocksRequests) {
  SetCapabilityForDevice(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
      test_eligible_external_devices_infos_[0], true /* enable */);
  SetCapabilityForDevice(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
      test_eligible_external_devices_infos_[1], true /* enable */);
  SetCapabilityForDevice(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
      test_eligible_external_devices_infos_[2], true /* enable */);

  InvokeErrorCallback();
  EXPECT_EQ(kErrorOnToggleEasyUnlockResult, GetResultAndReset());

  InvokeSetCapabilityKeyUnlockCallback();
  EXPECT_EQ(kSuccessResult, GetResultAndReset());

  InvokeSetCapabilityKeyUnlockCallback();
  EXPECT_EQ(kSuccessResult, GetResultAndReset());
}

TEST_F(DeviceCapabilityManagerTest,
       TestMultipleFindEligibleForUnlockDevicesRequests) {
  FindEligibleDevices(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY);
  FindEligibleDevices(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY);
  FindEligibleDevices(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY);

  InvokeFindEligibleUnlockDevicesCallback();
  EXPECT_EQ(kSuccessResult, GetResultAndReset());
  VerifyReturnedDevices(result_eligible_devices_, result_ineligible_devices_);

  InvokeErrorCallback();
  EXPECT_EQ(kErrorFindEligibleUnlockDevices, GetResultAndReset());

  InvokeFindEligibleUnlockDevicesCallback();
  EXPECT_EQ(kSuccessResult, GetResultAndReset());
  VerifyReturnedDevices(result_eligible_devices_, result_ineligible_devices_);
}

TEST_F(DeviceCapabilityManagerTest, TestMultipleIsPromotableRequests) {
  IsPromotableDevice(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                     test_eligible_external_devices_infos_[0].public_key());
  IsPromotableDevice(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                     test_eligible_external_devices_infos_[1].public_key());
  IsPromotableDevice(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                     test_eligible_external_devices_infos_[2].public_key());

  InvokeFindEligibleForPromotionCallback(true /*eligible*/);
  EXPECT_TRUE(GetEligibleForPromotionAndReset());

  InvokeFindEligibleForPromotionCallback(true /*eligible*/);
  EXPECT_TRUE(GetEligibleForPromotionAndReset());

  InvokeErrorCallback();
  EXPECT_EQ(kErrorFindEligibleForPromotion, GetResultAndReset());
}

TEST_F(DeviceCapabilityManagerTest, TestOrderViaMultipleErrors) {
  SetCapabilityForDevice(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
      test_eligible_external_devices_infos_[0], true /* enable */);
  FindEligibleDevices(
      DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY);
  IsPromotableDevice(DeviceCapabilityManager::Capability::CAPABILITY_UNLOCK_KEY,
                     test_eligible_external_devices_infos_[0].public_key());

  InvokeErrorCallback();
  EXPECT_EQ(kErrorOnToggleEasyUnlockResult, GetResultAndReset());

  InvokeErrorCallback();
  EXPECT_EQ(kErrorFindEligibleUnlockDevices, GetResultAndReset());

  InvokeErrorCallback();
  EXPECT_EQ(kErrorFindEligibleForPromotion, GetResultAndReset());
}

}  // namespace cryptauth