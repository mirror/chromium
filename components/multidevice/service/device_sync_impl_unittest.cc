// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/device_sync_impl.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/cryptauth_enroller.h"
#include "components/cryptauth/cryptauth_enrollment_manager.h"

#include "components/cryptauth/fake_cryptauth_gcm_manager.h"
#include "components/cryptauth/fake_remote_device_provider.h"
#include "components/cryptauth/fake_secure_message_delegate.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device_provider_impl.h"

#include "components/signin/core/browser/account_info.h"

#include "services/identity/public/cpp/account_state.h"

#include "services/identity/identity_service.h"
#include "services/identity/public/cpp/account_state.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"
#include "services/preferences/public/interfaces/preferences.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Invoke;
using testing::NiceMock;
using testing::Return;

namespace multidevice {

namespace {
const std::string kRegistrationId = "registrationId";
const std::string kAccountId = "accountId";

class FakeSecureMessageDelegateFactory
    : public cryptauth::SecureMessageDelegateFactory {
 public:
  // cryptauth::SecureMessageDelegateFactory:
  ~FakeSecureMessageDelegateFactory() override {
    LOG(ERROR) << "~FakeRemoteDeviceProviderFactory called";
  }

  std::unique_ptr<cryptauth::SecureMessageDelegate>
  CreateSecureMessageDelegate() override {
    return base::WrapUnique(new cryptauth::FakeSecureMessageDelegate());
  }
};

class FakeRemoteDeviceProviderFactory
    : public cryptauth::RemoteDeviceProviderImpl::Factory {
 public:
  FakeRemoteDeviceProviderFactory() {
    cryptauth::RemoteDeviceProviderImpl::Factory::SetInstanceForTesting(this);
  }
  ~FakeRemoteDeviceProviderFactory() {
    LOG(ERROR) << "~FakeRemoteDeviceProviderFactory called";
  }

 protected:
  std::unique_ptr<cryptauth::RemoteDeviceProvider> BuildInstance(
      cryptauth::CryptAuthDeviceManager* device_manager,
      const std::string& user_id,
      const std::string& user_private_key,
      cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory)
      override {
    LOG(ERROR) << "BuildInstance called!!!";
    return base::MakeUnique<cryptauth::FakeRemoteDeviceProvider>();
  }
};

}  // namespace

class FakeServiceContextRef : public service_manager::ServiceContextRef {
 public:
  FakeServiceContextRef() {}
  ~FakeServiceContextRef() override {}
  std::unique_ptr<ServiceContextRef> Clone() override { return nullptr; }
};

class MockCryptAuthEnrollmentManager
    : public cryptauth::CryptAuthEnrollmentManager {
 public:
  explicit MockCryptAuthEnrollmentManager(
      cryptauth::CryptAuthGCMManager* fake_cryptauth_gcm_manager)
      : cryptauth::CryptAuthEnrollmentManager(
            nullptr /* clock */,
            nullptr /* enroller_factory */,
            nullptr /* secure_message_delegate */,
            cryptauth::GcmDeviceInfo(),
            fake_cryptauth_gcm_manager,
            nullptr /* pref_service */) {}

  ~MockCryptAuthEnrollmentManager() override {
    LOG(ERROR) << "~MockCryptAuthEnrollmentManager called";
  }

  MOCK_CONST_METHOD0(IsEnrollmentValid, bool());
  MOCK_CONST_METHOD0(GetUserPrivateKey, std::string());
};

class MockDeviceManager : public cryptauth::CryptAuthDeviceManager {
 public:
  // Used: ForceSyncNow, AddObserver, RemoveObserver
  MockDeviceManager() {}
  ~MockDeviceManager() override { LOG(ERROR) << "~MockDeviceManager"; }

  MOCK_CONST_METHOD0(GetSyncResult,
                     cryptauth::CryptAuthDeviceManager::SyncResult());
  MOCK_CONST_METHOD0(GetDeviceChangeResult,
                     cryptauth::CryptAuthDeviceManager::DeviceChangeResult());

  // TODO(hsuregan): check for invocation reason.
  void ForceSyncNow(cryptauth::InvocationReason invocation_reason) override {
    LOG(ERROR) << "force sync now called from mockdevicemanager";
    cryptauth::CryptAuthDeviceManager::NotifySyncFinished(
        GetSyncResult(), GetDeviceChangeResult());
  }
};

class MockIdentityManager : public identity::mojom::IdentityManager {
 public:
  MockIdentityManager() {}

  ~MockIdentityManager() override { LOG(ERROR) << "~MockIdentityManager"; }

  MOCK_CONST_METHOD0(GetAccountInfo, AccountInfo());

  void GetPrimaryAccountInfo(GetPrimaryAccountInfoCallback callback) override {}
  void GetAccountInfoFromGaiaId(
      const std::string& gaia_id,
      GetAccountInfoFromGaiaIdCallback callback) override {}
  void GetAccounts(GetAccountsCallback callback) override {}
  void GetAccessToken(const std::string& account_id,
                      const identity::ScopeSet& scopes,
                      const std::string& consumer_id,
                      GetAccessTokenCallback callback) override {}

  void GetPrimaryAccountWhenAvailable(
      GetPrimaryAccountWhenAvailableCallback callback) override {
    AccountInfo account_info_ = GetAccountInfo();
    account_info_.account_id = kAccountId;
    identity::AccountState account_state_ = identity::AccountState();
    std::move(callback).Run(account_info_, account_state_);
  }
};

class DeviceSyncImpltest : public testing::Test {
 public:
  DeviceSyncImpltest() {}

  ~DeviceSyncImpltest() override { LOG(ERROR) << "~DeviceSyncImpltes called"; }

  void SetUp() override {
    fake_secure_message_delegate_factory_ =
        base::MakeUnique<FakeSecureMessageDelegateFactory>();

    fake_remote_device_provider_factory_ =
        base::MakeUnique<FakeRemoteDeviceProviderFactory>();

    fake_gcm_manager_ =
        base::MakeUnique<cryptauth::FakeCryptAuthGCMManager>("Test");

    mock_identity_manager_ =
        base::WrapUnique(new NiceMock<MockIdentityManager>());
    ON_CALL(*mock_identity_manager_, GetAccountInfo())
        .WillByDefault(Return(account_info_));

    mock_enrollment_manager_ = base::WrapUnique(
        new NiceMock<MockCryptAuthEnrollmentManager>(fake_gcm_manager_.get()));
    // Just use EXPECT_CALL(mock_enrollment_manager_,
    // ForceEnrollmentNow(::INVOCATION_REASON_INITIALIZATION)) Or
    // EXPECT_CALL(mock_enrollment_manager_,
    // ForceEnrollmentNow(Invocationreason::INVOCATION_REASON_MANUAL))
    // MOCK_CONST_METHOD1(ForceEnrollmentNow, void(cryptauth::InvocationReason
    // invocation_reason));

    // ON_CALL(*mock_enrollment_manager_,
    // ForceEnrollmentNow(_)).WillByDefault(Invoke(this,
    // &DeviceSyncImpltest::ForceEnrollmentNow));
    ON_CALL(*mock_enrollment_manager_, IsEnrollmentValid())
        .WillByDefault(Return(enrollment_valid_));
    ON_CALL(*mock_enrollment_manager_, GetUserPrivateKey())
        .WillByDefault(Return(user_private_key_));

    mock_device_manager_ = base::WrapUnique(new NiceMock<MockDeviceManager>());
    ON_CALL(*mock_device_manager_, GetSyncResult())
        .WillByDefault(Return(sync_result_));
    ON_CALL(*mock_device_manager_, GetDeviceChangeResult())
        .WillByDefault(Return(device_change_result_));
  }

  void ForceSyncNow(cryptauth::InvocationReason invocation_reason) {
    LOG(ERROR) << "force sync now called!";
  }

  void CreateDeviceSync() {
    device_sync_ = base::MakeUnique<DeviceSyncImpl>(
        base::MakeUnique<FakeServiceContextRef>(), mock_identity_manager_.get(),
        std::move(pref_connector_), fake_gcm_manager_.get(),
        mock_device_manager_.get(), mock_enrollment_manager_.get(),
        fake_remote_device_provider_factory_.get(),
        fake_secure_message_delegate_factory_.get());
    LOG(ERROR) << "done constructing!!";
  }

  AccountInfo account_info_;
  bool enrollment_valid_ = true;
  std::string user_private_key_;

  std::unique_ptr<cryptauth::FakeCryptAuthGCMManager> fake_gcm_manager_;

  std::unique_ptr<testing::NiceMock<MockIdentityManager>>
      mock_identity_manager_;

  prefs::mojom::PrefStoreConnectorPtr pref_connector_;

  std::unique_ptr<FakeSecureMessageDelegateFactory>
      fake_secure_message_delegate_factory_;

  std::unique_ptr<FakeRemoteDeviceProviderFactory>
      fake_remote_device_provider_factory_;

  std::unique_ptr<testing::NiceMock<MockCryptAuthEnrollmentManager>>
      mock_enrollment_manager_;

  std::unique_ptr<testing::NiceMock<MockDeviceManager>> mock_device_manager_;

  std::unique_ptr<DeviceSyncImpl> device_sync_;

  using SyncResult = cryptauth::CryptAuthDeviceManager::SyncResult;
  SyncResult sync_result_ = SyncResult::SUCCESS;

  using ChangeResult = cryptauth::CryptAuthDeviceManager::DeviceChangeResult;
  ChangeResult device_change_result_ = ChangeResult::CHANGED;
};

TEST_F(DeviceSyncImpltest, DryRun) {
  CreateDeviceSync();
  LOG(ERROR) << "test done!!";
}

}  // namespace multidevice