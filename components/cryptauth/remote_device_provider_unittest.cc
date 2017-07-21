// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "components/cryptauth/remote_device_provider.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
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

  void NotifySyncFinished() {
    cryptauth::CryptAuthDeviceManager::NotifySyncFinished(
        CryptAuthDeviceManager::SyncResult::SUCCESS,
        CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
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
  TestObserver() { num_times_device_list_changed_ = 0; }
  ~TestObserver() override {}
  // RemoteDeviceProvider::Observer:
  void OnSyncDeviceListchanged() override { num_times_device_list_changed_++; }

  int num_times_device_list_changed() { return num_times_device_list_changed_; }

 private:
  int num_times_device_list_changed_;
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
      EXPECT_TRUE(!callback_.is_null());

      std::vector<RemoteDevice> devices;
      for (const auto& remote_device : test_->test_devices_) {
        for (const auto& external_device_info : device_info_list) {
          if (remote_device.public_key == external_device_info.public_key())
            devices.push_back(remote_device);
        }
      }

      callback_.Run(devices);
    }

   private:
    cryptauth::RemoteDeviceLoader::RemoteDeviceCallback callback_;
    RemoteDeviceProviderTest* test_;
  };

  RemoteDeviceProviderTest()
      : test_devices_(cryptauth::GenerateTestRemoteDevices(5)),
        test_device_infos_(
            CreateExternalDeviceInfosForRemoteDevices(test_devices_)) {}

  std::vector<ExternalDeviceInfo> mock_device_manager_sync_devices() {
    return mock_device_manager_synced_device_infos_;
  }

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
    remote_device_provider_ = base::MakeUnique<RemoteDeviceProvider>(
        mock_device_manager_.get(), kTestUserId, kTestUserPrivateKey,
        fake_secure_message_delegate_factory_.get());
    base::RunLoop().RunUntilIdle();
    EXPECT_EQ(0u, remote_device_provider_->GetSyncedDevices().size());
    for (const auto& test_device_info : test_device_infos_) {
      mock_device_manager_synced_device_infos_.push_back(test_device_info);
    }
    mock_device_manager_->NotifySyncFinished();
    test_device_loader_factory_->InvokeLastCallback(
        mock_device_manager_synced_device_infos_);
    test_observer_ = base::MakeUnique<TestObserver>();
    base::RunLoop().RunUntilIdle();
  }

  const base::test::ScopedTaskEnvironment scoped_task_environment_;
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

 protected:
  DISALLOW_COPY_AND_ASSIGN(RemoteDeviceProviderTest);
};

TEST_F(RemoteDeviceProviderTest, TestOriginalDevicesSynced) {
  std::vector<cryptauth::RemoteDevice> saved_devices =
      remote_device_provider_->GetSyncedDevices();
  int len = saved_devices.size();
  EXPECT_EQ(static_cast<int>(test_devices_.size()), len);
  std::set<cryptauth::RemoteDevice> set_saved_devices(saved_devices.begin(),
                                                      saved_devices.end());
  for (const auto& test_device : test_devices_) {
    EXPECT_TRUE(set_saved_devices.find(test_device) != set_saved_devices.end());
  }
}

TEST_F(RemoteDeviceProviderTest, TestObserversNotifiedOfChange) {
  remote_device_provider_->AddObserver(test_observer_.get());
  mock_device_manager_->NotifySyncFinished();
  test_device_loader_factory_->InvokeLastCallback(
      mock_device_manager_synced_device_infos_);
  EXPECT_EQ(1, test_observer_->num_times_device_list_changed());
  EXPECT_EQ(remote_device_provider_->GetSyncedDevices().size(),
            mock_device_manager_->GetSyncedDevices().size());
}

TEST_F(RemoteDeviceProviderTest, TestLatestResult) {
  remote_device_provider_->AddObserver(test_observer_.get());
  mock_device_manager_->NotifySyncFinished();
  mock_device_manager_synced_device_infos_.pop_back();
  mock_device_manager_->GetSyncedDevices();
  mock_device_manager_->NotifySyncFinished();
  test_device_loader_factory_->InvokeLastCallback(
      mock_device_manager_synced_device_infos_);
  EXPECT_EQ(1, test_observer_->num_times_device_list_changed());
  EXPECT_EQ(remote_device_provider_->GetSyncedDevices().size(),
            mock_device_manager_->GetSyncedDevices().size());
}

}  // namespace cryptauth
