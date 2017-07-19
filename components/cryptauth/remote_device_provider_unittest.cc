// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "components/cryptauth/remote_device_provider.h"

#include <memory>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/cryptauth_enroller.h"
#include "components/cryptauth/cryptauth_enrollment_manager.h"
#include "components/cryptauth/fake_cryptauth_gcm_manager.h"
#include "components/cryptauth/fake_cryptauth_service.h"
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
// securemessage override

class MockCryptAuthDeviceManager : public ::cryptauth::CryptAuthDeviceManager {
 public:
  ~MockCryptAuthDeviceManager() override {}

  MOCK_CONST_METHOD0(GetSyncedDevices,
                     std::vector<::cryptauth::ExternalDeviceInfo>());
};

class FakeSecureMessageDelegateFactory
    : public ::cryptauth::SecureMessageDelegateFactory {
 public:
  // FakeCryptAuthService:
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

// no change
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

// no change
std::vector<cryptauth::ExternalDeviceInfo>
CreateTetherExternalDeviceInfosForRemoteDevices(
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

class FakeObserver : public RemoteDeviceProvider::Observer,
                     public cryptauth::CryptAuthDeviceManager::Observer {
 public:
  FakeObserver() : numTimesOnSyncDeviceListchangedCalled(0) {}
  void OnSyncDeviceListchanged() override;
  ~FakeObserver() override{};
  int numTimesOnSyncDeviceListchangedCalled;
};

void FakeObserver::OnSyncDeviceListchanged() {
  numTimesOnSyncDeviceListchangedCalled++;
}

}  // namespace

// naming
class RemoteDeviceProviderTest : public testing::Test {
 public:
  // keep class
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
      EXPECT_EQ(test_->test_device_infos_.size(), device_info_list.size());
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

    void InvokeLastCallback() {
      DCHECK(!callback_.is_null());
      callback_.Run(test_->test_devices_);
    }

   private:
    cryptauth::RemoteDeviceLoader::RemoteDeviceCallback callback_;
    RemoteDeviceProviderTest* test_;
  };

  RemoteDeviceProviderTest()
      : test_devices_(cryptauth::GenerateTestRemoteDevices(5)),
        test_device_infos_(
            CreateTetherExternalDeviceInfosForRemoteDevices(test_devices_)) {}

  void SetUp() override {
    device_list_list_.clear();
    single_device_list_.clear();
    mock_device_manager_ =
        base::WrapUnique(new NiceMock<MockCryptAuthDeviceManager>());
    ON_CALL(*mock_device_manager_, GetSyncedDevices())
        .WillByDefault(Return(test_device_infos_));

    fake_secure_message_delegate_factory_ =
        base::MakeUnique<FakeSecureMessageDelegateFactory>();

    remote_device_provider_ = base::MakeUnique<RemoteDeviceProvider>(
        mock_device_manager_.get(), kTestUserId, kTestUserPrivateKey,
        fake_secure_message_delegate_factory_.get());
  }

  void OnTetherHostListFetched(const cryptauth::RemoteDeviceList& device_list) {
    device_list_list_.push_back(device_list);
  }

  void OnSingleTetherHostFetched(
      std::unique_ptr<cryptauth::RemoteDevice> device) {
    single_device_list_.push_back(std::move(device));
  }

  const std::vector<cryptauth::RemoteDevice> test_devices_;
  const std::vector<cryptauth::ExternalDeviceInfo> test_device_infos_;

  std::vector<cryptauth::RemoteDeviceList> device_list_list_;
  std::vector<std::shared_ptr<cryptauth::RemoteDevice>> single_device_list_;

  std::unique_ptr<FakeSecureMessageDelegateFactory>
      fake_secure_message_delegate_factory_;

  std::unique_ptr<NiceMock<MockCryptAuthDeviceManager>> mock_device_manager_;

  std::unique_ptr<TestRemoteDeviceLoaderFactory> test_device_loader_factory_;

  std::unique_ptr<RemoteDeviceProvider> remote_device_provider_;

 protected:
  DISALLOW_COPY_AND_ASSIGN(RemoteDeviceProviderTest);
};

TEST_F(RemoteDeviceProviderTest, TestOnSyncCalls) {
  auto test_observer = base::MakeUnique<FakeObserver>();
  remote_device_provider_->AddObserver(test_observer.get());
  remote_device_provider_->OnSyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  EXPECT_EQ(1, test_observer->numTimesOnSyncDeviceListchangedCalled);
  remote_device_provider_->OnSyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  EXPECT_EQ(2, test_observer->numTimesOnSyncDeviceListchangedCalled);
}

TEST_F(RemoteDeviceProviderTest, TestRemoteDeviceListChange) {
  EXPECT_EQ(0u, remote_device_provider_->GetSyncedDevices().size());
  remote_device_provider_->OnSyncFinished(
      CryptAuthDeviceManager::SyncResult::SUCCESS,
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED);
  std::vector<cryptauth::RemoteDevice> saved_devices =
      remote_device_provider_->GetSyncedDevices();
  int len = saved_devices.size();
  EXPECT_EQ(5, len);
  for (int i = 0; i < len; i++) {
    EXPECT_EQ(test_devices_[i].GetDeviceId(), saved_devices[i].GetDeviceId());
  }
}

}  // namespace cryptauth
