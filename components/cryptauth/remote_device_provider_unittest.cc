// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/remote_device_provider.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/fake_secure_message_delegate.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device_loader.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;

namespace cryptauth {

namespace {

const char kTestUserId[] = "testUserId";
const char kTestUserPrivateKey[] = "kTestUserPrivateKey";

class MockCryptAuthDeviceManager : public cryptauth::CryptAuthDeviceManager {
 public:
  ~MockCryptAuthDeviceManager() override {}

  void NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult sync_result,
      CryptAuthDeviceManager::DeviceChangeResult device_change_result) {
    cryptauth::CryptAuthDeviceManager::NotifySyncFinished(sync_result,
                                                          device_change_result);
  }

  MOCK_CONST_METHOD0(GetSyncedDevices,
                     std::vector<::cryptauth::ExternalDeviceInfo>());
};

class FakeSecureMessageDelegateFactory
    : public cryptauth::SecureMessageDelegateFactory {
 public:
  // cryptauth::SecureMessageDelegateFactory:
  std::unique_ptr<cryptauth::SecureMessageDelegate>
  CreateSecureMessageDelegate() override {
    cryptauth::FakeSecureMessageDelegate* delegate =
        new cryptauth::FakeSecureMessageDelegate();
    created_delegates_.push_back(delegate);
    return base::WrapUnique(delegate);
  }

  void VerifySecureMessageDelegateCreatedByFactory(
      ::cryptauth::FakeSecureMessageDelegate* delegate) {
    EXPECT_FALSE(std::find(created_delegates_.begin(), created_delegates_.end(),
                           delegate) == created_delegates_.end());
  }

 private:
  std::vector<cryptauth::FakeSecureMessageDelegate*> created_delegates_;
};

class MockDeviceLoader : public cryptauth::RemoteDeviceLoader {
 public:
  MockDeviceLoader()
      : cryptauth::RemoteDeviceLoader(
            std::vector<cryptauth::ExternalDeviceInfo>(),
            "",
            "",
            nullptr) {}
  ~MockDeviceLoader() override {}

  MOCK_METHOD2(
      Load,
      void(bool, const cryptauth::RemoteDeviceLoader::RemoteDeviceCallback&));
};

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

class TestObserver : public RemoteDeviceProvider::Observer {
 public:
  TestObserver() {}
  ~TestObserver() override {}

  int num_times_device_list_changed() { return num_times_device_list_changed_; }

  // RemoteDeviceProvider::Observer:
  void OnSyncDeviceListChanged() override { num_times_device_list_changed_++; }

 private:
  int num_times_device_list_changed_ = 0;
};

}  // namespace

class RemoteDeviceProviderTest : public testing::Test {
 public:
  class TestRemoteDeviceLoaderFactory : public RemoteDeviceLoader::Factory {
   public:
    explicit TestRemoteDeviceLoaderFactory(RemoteDeviceProviderTest* test)
        : test_(test) {}

    std::unique_ptr<RemoteDeviceLoader> BuildInstance(
        const std::vector<cryptauth::ExternalDeviceInfo>& device_info_list,
        const std::string& user_id,
        const std::string& user_private_key,
        std::unique_ptr<cryptauth::SecureMessageDelegate>
            secure_message_delegate) override {
      EXPECT_EQ(test_->mock_device_manager_synced_device_infos_.size(),
                device_info_list.size());
      EXPECT_EQ(std::string(kTestUserId), user_id);
      EXPECT_EQ(std::string(kTestUserPrivateKey), user_private_key);
      test_->fake_secure_message_delegate_factory_
          ->VerifySecureMessageDelegateCreatedByFactory(
              static_cast<cryptauth::FakeSecureMessageDelegate*>(
                  secure_message_delegate.get()));

      std::unique_ptr<MockDeviceLoader> device_loader =
          base::WrapUnique(new NiceMock<MockDeviceLoader>());
      ON_CALL(*device_loader, Load(false, _))
          .WillByDefault(
              Invoke(this, &TestRemoteDeviceLoaderFactory::MockLoadImpl));
      return std::move(device_loader);
    }

    void MockLoadImpl(
        bool should_load_beacon_seeds,
        const cryptauth::RemoteDeviceLoader::RemoteDeviceCallback& callback) {
      callback_ = callback;
    }

    void InvokeLastCallback(
        const std::vector<cryptauth::ExternalDeviceInfo>& device_info_list) {
      ASSERT_TRUE(!callback_.is_null());

      // Fetch only the devices inserted by tests, since test_devices_ contains
      // all available devices.
      std::vector<RemoteDevice> devices;
      for (const auto& remote_device : test_->test_devices_) {
        for (const auto& external_device_info : device_info_list) {
          if (remote_device.public_key == external_device_info.public_key())
            devices.push_back(remote_device);
        }
      }

      callback_.Run(devices);
      callback_.Reset();
    }

    // Fetch is only started if the change result passed to OnSyncFinished() is
    // CHANGED and sync is SUCCESS.
    bool HasQueuedCallback() { return !callback_.is_null(); }

   private:
    cryptauth::RemoteDeviceLoader::RemoteDeviceCallback callback_;
    RemoteDeviceProviderTest* test_;
  };

  RemoteDeviceProviderTest()
      : test_devices_(cryptauth::GenerateTestRemoteDevices(5)),
        test_device_infos_(
            CreateExternalDeviceInfosForRemoteDevices(test_devices_)) {}

  void SetUp() override {
    mock_device_manager_ =
        base::WrapUnique(new NiceMock<MockCryptAuthDeviceManager>());
    ON_CALL(*mock_device_manager_, GetSyncedDevices())
        .WillByDefault(Invoke(
            this, &RemoteDeviceProviderTest::mock_device_manager_sync_devices));
    fake_secure_message_delegate_factory_ =
        base::MakeUnique<FakeSecureMessageDelegateFactory>();
    test_device_loader_factory_ =
        base::WrapUnique(new TestRemoteDeviceLoaderFactory(this));
    cryptauth::RemoteDeviceLoader::Factory::SetInstanceForTesting(
        test_device_loader_factory_.get());
    test_observer_ = base::MakeUnique<TestObserver>();
  }

  void CreateRemoteDeviceProvider() {
    remote_device_provider_ = base::MakeUnique<RemoteDeviceProvider>(
        mock_device_manager_.get(), kTestUserId, kTestUserPrivateKey,
        fake_secure_message_delegate_factory_.get());
    remote_device_provider_->AddObserver(test_observer_.get());
    EXPECT_EQ(0u, remote_device_provider_->GetSyncedDevices().size());
    test_device_loader_factory_->InvokeLastCallback(
        mock_device_manager_synced_device_infos_);
    VerifySyncedDevicesMatchExpectation(
        mock_device_manager_synced_device_infos_.size());
  }

  void VerifySyncedDevicesMatchExpectation(size_t expected_size) {
    std::vector<cryptauth::RemoteDevice> synced_devices =
        remote_device_provider_->GetSyncedDevices();
    EXPECT_EQ(expected_size, synced_devices.size());
    std::unordered_set<std::string> public_keys;
    for (const auto& device_info : mock_device_manager_synced_device_infos_) {
      public_keys.insert(device_info.public_key());
    }
    for (const auto& remote_device : synced_devices) {
      EXPECT_TRUE(public_keys.find(remote_device.public_key) !=
                  public_keys.end());
    }
  }

  // This is the mock implementation of mock_device_manager_'s
  // GetSyncedDevices().
  std::vector<ExternalDeviceInfo> mock_device_manager_sync_devices() {
    return mock_device_manager_synced_device_infos_;
  }

  const std::vector<cryptauth::RemoteDevice> test_devices_;
  const std::vector<cryptauth::ExternalDeviceInfo> test_device_infos_;

  std::vector<cryptauth::ExternalDeviceInfo>
      mock_device_manager_synced_device_infos_;

  std::unique_ptr<FakeSecureMessageDelegateFactory>
      fake_secure_message_delegate_factory_;

  std::unique_ptr<NiceMock<MockCryptAuthDeviceManager>> mock_device_manager_;

  std::unique_ptr<TestRemoteDeviceLoaderFactory> test_device_loader_factory_;

  std::unique_ptr<RemoteDeviceProvider> remote_device_provider_;

  std::unique_ptr<TestObserver> test_observer_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RemoteDeviceProviderTest);
};

TEST_F(RemoteDeviceProviderTest, TestMultipleSyncs) {
  // Initialize with devices 0 and 1.
  mock_device_manager_synced_device_infos_ = std::vector<ExternalDeviceInfo>{
      test_device_infos_[0], test_device_infos_[1]};
  CreateRemoteDeviceProvider();
  VerifySyncedDevicesMatchExpectation(2u /* expected_size */);
  EXPECT_EQ(1, test_observer_->num_times_device_list_changed());

  // Now add device 2 and trigger another sync.
  mock_device_manager_synced_device_infos_.push_back(test_device_infos_[2]);
  mock_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  test_device_loader_factory_->InvokeLastCallback(
      mock_device_manager_synced_device_infos_);
  VerifySyncedDevicesMatchExpectation(3u /* expected_size */);
  EXPECT_EQ(2, test_observer_->num_times_device_list_changed());

  // // Now, simulate a sync which shows that device 0 was removed.
  mock_device_manager_synced_device_infos_.erase(
      mock_device_manager_synced_device_infos_.begin());
  mock_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  test_device_loader_factory_->InvokeLastCallback(
      mock_device_manager_synced_device_infos_);
  VerifySyncedDevicesMatchExpectation(2u /* expected_size */);
  EXPECT_EQ(3, test_observer_->num_times_device_list_changed());
}

TEST_F(RemoteDeviceProviderTest, TestFailureModes) {
  mock_device_manager_synced_device_infos_.push_back(test_device_infos_[0]);
  CreateRemoteDeviceProvider();
  VerifySyncedDevicesMatchExpectation(1u /* expected_size */);
  mock_device_manager_synced_device_infos_.push_back(test_device_infos_[1]);
  mock_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::FAILURE,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  EXPECT_FALSE(test_device_loader_factory_->HasQueuedCallback());
  VerifySyncedDevicesMatchExpectation(1u /* expected_size */);
  EXPECT_EQ(1, test_observer_->num_times_device_list_changed());

  mock_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::UNCHANGED);
  EXPECT_FALSE(test_device_loader_factory_->HasQueuedCallback());
  VerifySyncedDevicesMatchExpectation(1u /* expected_size */);
  EXPECT_EQ(1, test_observer_->num_times_device_list_changed());

  mock_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::FAILURE,
      CryptAuthDeviceManager::DeviceChangeResult::UNCHANGED);
  EXPECT_FALSE(test_device_loader_factory_->HasQueuedCallback());
  VerifySyncedDevicesMatchExpectation(1u /* expected_size */);
  EXPECT_EQ(1, test_observer_->num_times_device_list_changed());

  mock_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  test_device_loader_factory_->InvokeLastCallback(
      mock_device_manager_synced_device_infos_);
  VerifySyncedDevicesMatchExpectation(2u /* expected_size */);
  EXPECT_EQ(2, test_observer_->num_times_device_list_changed());
}

TEST_F(RemoteDeviceProviderTest, TestNewSyncDuringDeviceRegeneration) {
  mock_device_manager_synced_device_infos_.push_back(test_device_infos_[0]);
  CreateRemoteDeviceProvider();
  VerifySyncedDevicesMatchExpectation(1u /* expected_size */);
  mock_device_manager_synced_device_infos_.push_back(test_device_infos_[1]);
  mock_device_manager_->NotifySyncFinished(
      CryptAuthDeviceManager::SyncResult::FAILURE,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  EXPECT_NE(mock_device_manager_synced_device_infos_.size(),
            remote_device_provider_->GetSyncedDevices().size());
}

}  // namespace cryptauth
