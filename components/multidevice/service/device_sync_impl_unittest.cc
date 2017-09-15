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
using SyncResult = cryptauth::CryptAuthDeviceManager::SyncResult;
using ChangeResult = cryptauth::CryptAuthDeviceManager::DeviceChangeResult;
using InvocationReason = cryptauth::InvocationReason;

namespace multidevice {

namespace {
const std::string kRegistrationId = "registrationId";
const std::string kAccountId = "accountId";
const std::string kUserPrivateKey = "userPrivateKey";
}  // namespace

class FakeSecureMessageDelegateFactory
    : public cryptauth::SecureMessageDelegateFactory {
 public:
  // cryptauth::SecureMessageDelegateFactory:
  ~FakeSecureMessageDelegateFactory() override {
    LOG(ERROR) << "~FakeSecureMessageDelegateFactory called";
  }

  std::unique_ptr<cryptauth::SecureMessageDelegate>
  CreateSecureMessageDelegate() override {
    return base::WrapUnique(new cryptauth::FakeSecureMessageDelegate());
  }
};

class FakeRemoteDeviceProviderFactory
    : public cryptauth::RemoteDeviceProviderImpl::Factory {
 public:
  FakeRemoteDeviceProviderFactory() {}
  ~FakeRemoteDeviceProviderFactory() override {
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

class FakeServiceContextRef : public service_manager::ServiceContextRef {
 public:
  FakeServiceContextRef() {}
  ~FakeServiceContextRef() override {}
  std::unique_ptr<ServiceContextRef> Clone() override { return nullptr; }
};

class FakeCryptAuthEnrollmentManager
    : public cryptauth::CryptAuthEnrollmentManager {
 public:
  explicit FakeCryptAuthEnrollmentManager(
      cryptauth::CryptAuthGCMManager* fake_cryptauth_gcm_manager)
      : cryptauth::CryptAuthEnrollmentManager(
            nullptr /* clock */,
            nullptr /* enroller_factory */,
            nullptr /* secure_message_delegate */,
            cryptauth::GcmDeviceInfo(),
            fake_cryptauth_gcm_manager,
            nullptr /* pref_service */) {}

  ~FakeCryptAuthEnrollmentManager() override {
    LOG(ERROR) << "~FakeCryptAuthEnrollmentManager called";
  }

  void ForceEnrollmentNow(InvocationReason invocation_reason) override {
    expected_invocation_reason_ = invocation_reason;
    cryptauth::CryptAuthEnrollmentManager::NotifyEnrollmentFinished(
        expected_enrollment_status_);
  }

  bool IsEnrollmentValid() const override {
    return expected_enrollment_status_;
  }

  std::string GetUserPrivateKey() const override { return user_private_key_; }

  void SetExpectedEnrollmentStatus(bool status) {
    expected_enrollment_status_ = status;
  }

  InvocationReason GetAndClearInvocationReason() {
    InvocationReason invocation_reason = expected_invocation_reason_;
    expected_invocation_reason_ = InvocationReason::INVOCATION_REASON_UNKNOWN;
    return invocation_reason;
  }

 private:
  std::string user_private_key_ = kUserPrivateKey;
  bool expected_enrollment_status_ = true;
  InvocationReason expected_invocation_reason_ =
      InvocationReason::INVOCATION_REASON_UNKNOWN;
};

class FakeCryptAuthDeviceManager : public cryptauth::CryptAuthDeviceManager {
 public:
  // Used: ForceSyncNow, AddObserver, RemoveObserver
  FakeCryptAuthDeviceManager() {}
  ~FakeCryptAuthDeviceManager() override {
    LOG(ERROR) << "~FakeCryptAuthDeviceManager";
  }

  void ForceSyncNow(InvocationReason invocation_reason) override {
    expected_invocation_reason_ = invocation_reason;
    cryptauth::CryptAuthDeviceManager::NotifySyncFinished(
        expected_sync_result_, expected_device_change_result_);
  }

  void SetExpectedSyncResult(SyncResult sync_result) {
    expected_sync_result_ = sync_result;
  }

  void SetExpectedChangeResult(DeviceChangeResult device_change_result) {
    expected_device_change_result_ = device_change_result;
  }

  InvocationReason GetAndClearInvocationReason() {
    InvocationReason invocation_reason = expected_invocation_reason_;
    expected_invocation_reason_ = InvocationReason::INVOCATION_REASON_UNKNOWN;
    return invocation_reason;
  }

 private:
  SyncResult expected_sync_result_ = SyncResult::SUCCESS;
  DeviceChangeResult expected_device_change_result_ =
      DeviceChangeResult::CHANGED;
  InvocationReason expected_invocation_reason_ =
      InvocationReason::INVOCATION_REASON_UNKNOWN;
};

class FakeIdentityManager : public identity::mojom::IdentityManager {
 public:
  FakeIdentityManager() { LOG(ERROR) << "Mock ctr"; }
  ~FakeIdentityManager() override { LOG(ERROR) << "~FakeIdentityManager"; }

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
    info_.account_id = kAccountId;
    identity::AccountState account_state_ = identity::AccountState();
    std::move(callback).Run(info_, account_state_);
  }

  void set_account_info(AccountInfo info) { info_ = info; }

 private:
  AccountInfo info_;
};

class FakePrefStoreConnector : public prefs::mojom::PrefStoreConnector {
 public:
  FakePrefStoreConnector() {}

  ~FakePrefStoreConnector() override {
    LOG(ERROR) << "~FakePrefStoreConnector";
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
    cryptauth::RemoteDeviceProviderImpl::Factory::SetInstanceForTesting(
        fake_remote_device_provider_factory_.get());

    fake_gcm_manager_ =
        base::MakeUnique<cryptauth::FakeCryptAuthGCMManager>(kRegistrationId);

    fake_identity_manager_ = base::WrapUnique(new FakeIdentityManager());
    AccountInfo account_info;
    account_info.account_id = kAccountId;
    fake_identity_manager_->set_account_info(account_info);

    fake_enrollment_manager_ = base::WrapUnique(
        new NiceMock<FakeCryptAuthEnrollmentManager>(fake_gcm_manager_.get()));

    fake_device_manager_ = base::WrapUnique(new FakeCryptAuthDeviceManager());
  }

  void TearDown() override {}

  void ForceSyncNow(cryptauth::InvocationReason invocation_reason) {
    LOG(ERROR) << "force sync now called!";
  }

  void CreateDeviceSync() {
    device_sync_ = base::MakeUnique<DeviceSyncImpl>(
        base::MakeUnique<FakeServiceContextRef>(), fake_identity_manager_.get(),
        fake_gcm_manager_.get(), fake_device_manager_.get(),
        fake_enrollment_manager_.get(),
        fake_remote_device_provider_factory_.get(),
        fake_secure_message_delegate_factory_.get());
  }

  void ForceEnrollmentNow() { device_sync_->ForceEnrollmentNow(); }

  void ForceSyncNow() { device_sync_->ForceSyncNow(); }

  std::unique_ptr<cryptauth::FakeCryptAuthGCMManager> fake_gcm_manager_;

  std::unique_ptr<FakeSecureMessageDelegateFactory>
      fake_secure_message_delegate_factory_;

  std::unique_ptr<FakeIdentityManager> fake_identity_manager_;

  std::unique_ptr<FakeCryptAuthEnrollmentManager> fake_enrollment_manager_;

  std::unique_ptr<FakeRemoteDeviceProviderFactory>
      fake_remote_device_provider_factory_;

  std::unique_ptr<FakeCryptAuthDeviceManager> fake_device_manager_;

  std::unique_ptr<DeviceSyncImpl> device_sync_;

  SyncResult sync_result_ = SyncResult::SUCCESS;

  ChangeResult device_change_result_ = ChangeResult::CHANGED;
};

TEST_F(DeviceSyncImpltest, TestInjectedInitializationFlowSuccess) {
  EXPECT_FALSE(fake_device_manager_->GetAndClearInvocationReason());
  EXPECT_FALSE(fake_enrollment_manager_->GetAndClearInvocationReason());
  CreateDeviceSync();
  EXPECT_EQ(fake_device_manager_->GetAndClearInvocationReason(),
            InvocationReason::INVOCATION_REASON_INITIALIZATION);
  EXPECT_FALSE(fake_enrollment_manager_->GetAndClearInvocationReason());
}

TEST_F(DeviceSyncImpltest, TestForceEnrollment) {
  EXPECT_FALSE(fake_enrollment_manager_->GetAndClearInvocationReason());
  CreateDeviceSync();
  EXPECT_FALSE(fake_enrollment_manager_->GetAndClearInvocationReason());
  ForceEnrollmentNow();
  EXPECT_EQ(fake_enrollment_manager_->GetAndClearInvocationReason(),
            InvocationReason::INVOCATION_REASON_MANUAL);
}

TEST_F(DeviceSyncImpltest, TestForceSync) {
  EXPECT_FALSE(fake_device_manager_->GetAndClearInvocationReason());
  CreateDeviceSync();
  EXPECT_EQ(fake_device_manager_->GetAndClearInvocationReason(),
            InvocationReason::INVOCATION_REASON_INITIALIZATION);
  ForceSyncNow();
  EXPECT_EQ(fake_device_manager_->GetAndClearInvocationReason(),
            InvocationReason::INVOCATION_REASON_MANUAL);
}

}  // namespace multidevice