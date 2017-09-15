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
#include "components/cryptauth/remote_device_test_util.h"
#include "components/signin/core/browser/account_info.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/identity/identity_service.h"
#include "services/identity/public/cpp/account_state.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"
#include "services/preferences/public/interfaces/preferences.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "services/service_manager/public/cpp/service_test.h"
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
  ~FakeSecureMessageDelegateFactory() override {}

  std::unique_ptr<cryptauth::SecureMessageDelegate>
  CreateSecureMessageDelegate() override {
    return base::WrapUnique(new cryptauth::FakeSecureMessageDelegate());
  }
};

class FakeServiceContextRef : public service_manager::ServiceContextRef {
 public:
  ~FakeServiceContextRef() override {}
  std::unique_ptr<ServiceContextRef> Clone() override { return nullptr; }
};

class FakeManagerBase {
 public:
  ~FakeManagerBase() {}

  InvocationReason GetAndClearInvocationReason() {
    InvocationReason invocation_reason = expected_invocation_reason_;
    expected_invocation_reason_ = InvocationReason::INVOCATION_REASON_UNKNOWN;
    return invocation_reason;
  }

  void SetInvocationReason(InvocationReason invocation_reason) {
    expected_invocation_reason_ = invocation_reason;
  }

 protected:
  InvocationReason GetInvocationReason() { return expected_invocation_reason_; }

 private:
  InvocationReason expected_invocation_reason_ =
      InvocationReason::INVOCATION_REASON_UNKNOWN;
};

class FakeCryptAuthEnrollmentManager
    : public cryptauth::CryptAuthEnrollmentManager,
      public FakeManagerBase {
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

  ~FakeCryptAuthEnrollmentManager() override {}

  void ForceEnrollmentNow(InvocationReason invocation_reason) override {
    FakeManagerBase::SetInvocationReason(invocation_reason);
    cryptauth::CryptAuthEnrollmentManager::NotifyEnrollmentFinished(
        FakeManagerBase::GetInvocationReason());
  }

  bool IsEnrollmentValid() const override { return is_enrollment_valid_; }

  std::string GetUserPrivateKey() const override { return user_private_key_; }

  void SetIsEnrollmentValid(bool status) { is_enrollment_valid_ = status; }

 private:
  std::string user_private_key_ = kUserPrivateKey;
  bool is_enrollment_valid_ = true;
};

class FakeCryptAuthDeviceManager : public cryptauth::CryptAuthDeviceManager,
                                   public FakeManagerBase {
 public:
  // Used: ForceSyncNow, AddObserver, RemoveObserver
  FakeCryptAuthDeviceManager() {}
  ~FakeCryptAuthDeviceManager() override {}

  void ForceSyncNow(InvocationReason invocation_reason) override {
    FakeManagerBase::SetInvocationReason(invocation_reason);
    cryptauth::CryptAuthDeviceManager::NotifySyncFinished(
        expected_sync_result_, expected_device_change_result_);
  }

  void SetExpectedSyncResult(SyncResult sync_result) {
    expected_sync_result_ = sync_result;
  }

  void SetExpectedChangeResult(DeviceChangeResult device_change_result) {
    expected_device_change_result_ = device_change_result;
  }

 private:
  SyncResult expected_sync_result_ = SyncResult::SUCCESS;
  DeviceChangeResult expected_device_change_result_ =
      DeviceChangeResult::CHANGED;
};

class FakeIdentityManager : public identity::mojom::IdentityManager {
 public:
  FakeIdentityManager() {}
  ~FakeIdentityManager() override {}

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

class DeviceSyncImpltest : public service_manager::test::ServiceTest {
 public:
  class DeviceSyncObserverImpl : public device_sync::mojom::DeviceSyncObserver {
   public:
    DeviceSyncObserverImpl(
        device_sync::mojom::DeviceSyncObserverRequest request)
        : binding_(this, std::move(request)) {}

    void OnEnrollmentFinished(bool success) override {
      if (success) {
        num_times_enrollment_finished_called_.success_count++;
      } else {
        num_times_enrollment_finished_called_.failure_count++;
      }
    }

    void OnDevicesSynced(bool success, bool device_list_changed) override {
      if (success) {
        num_times_device_synced_.success_count++;
      } else {
        num_times_device_synced_.failure_count++;
      }
    }

    int GetNumCallsEnrollment(bool success_count) {
      return num_times_enrollment_finished_called_.CountType(success_count);
    }

    int GetNumCallsSync(bool success_count) {
      return num_times_device_synced_.CountType(success_count);
    }

   private:
    struct ObserverCallbackCount {
      int success_count = 0;
      int failure_count = 0;
      int CountType(bool success) {
        return success ? success_count : failure_count;
      }
    };

    mojo::Binding<device_sync::mojom::DeviceSyncObserver> binding_;

    ObserverCallbackCount num_times_enrollment_finished_called_;
    ObserverCallbackCount num_times_device_synced_;
  };

  DeviceSyncImpltest() {}

  void SetUp() override {
    // ServiceTest::SetUp();
    fake_secure_message_delegate_factory_ =
        base::MakeUnique<FakeSecureMessageDelegateFactory>();

    fake_gcm_manager_ =
        base::MakeUnique<cryptauth::FakeCryptAuthGCMManager>(kRegistrationId);

    fake_identity_manager_ = base::WrapUnique(new FakeIdentityManager());
    AccountInfo account_info;
    account_info.account_id = kAccountId;
    fake_identity_manager_->set_account_info(account_info);

    fake_enrollment_manager_ = base::WrapUnique(
        new NiceMock<FakeCryptAuthEnrollmentManager>(fake_gcm_manager_.get()));

    fake_device_manager_ = base::WrapUnique(new FakeCryptAuthDeviceManager());

    fake_remote_device_provider_ =
        base::WrapUnique(new cryptauth::FakeRemoteDeviceProvider());
  }

  void TearDown() override {}

  void CreateDeviceSync() {
    device_sync_ = base::MakeUnique<DeviceSyncImpl>(
        base::MakeUnique<FakeServiceContextRef>(), fake_identity_manager_.get(),
        fake_gcm_manager_.get(), fake_device_manager_.get(),
        fake_enrollment_manager_.get(), fake_remote_device_provider_.get(),
        fake_secure_message_delegate_factory_.get());
  }

  void GetSyncedDevicesCallback(
      const std::vector<cryptauth::RemoteDevice>& remote_devices) {
    std::unordered_set<std::string> public_keys;
    for (const auto& remote_device :
         fake_remote_device_provider_->GetSyncedDevices()) {
      public_keys.insert(remote_device.public_key);
    }
    for (const auto& remote_device : remote_devices) {
      EXPECT_TRUE(public_keys.find(remote_device.public_key) !=
                  public_keys.end());
    }
  }

  void ForceEnrollmentNow() { device_sync_->ForceEnrollmentNow(); }

  void ForceSyncNow() { device_sync_->ForceSyncNow(); }

  std::unique_ptr<cryptauth::FakeCryptAuthGCMManager> fake_gcm_manager_;

  std::unique_ptr<FakeSecureMessageDelegateFactory>
      fake_secure_message_delegate_factory_;

  std::unique_ptr<FakeIdentityManager> fake_identity_manager_;

  std::unique_ptr<FakeCryptAuthEnrollmentManager> fake_enrollment_manager_;

  std::unique_ptr<cryptauth::FakeRemoteDeviceProvider>
      fake_remote_device_provider_;

  std::unique_ptr<FakeCryptAuthDeviceManager> fake_device_manager_;

  std::unique_ptr<DeviceSyncObserverImpl> device_sync_observer_;

  std::unique_ptr<DeviceSyncImpl> device_sync_;
};

TEST_F(DeviceSyncImpltest, TestValidEnrollmentOnInit) {
  EXPECT_FALSE(fake_device_manager_->GetAndClearInvocationReason());
  EXPECT_FALSE(fake_enrollment_manager_->GetAndClearInvocationReason());
  fake_enrollment_manager_->SetIsEnrollmentValid(true);
  CreateDeviceSync();
  EXPECT_EQ(fake_device_manager_->GetAndClearInvocationReason(),
            InvocationReason::INVOCATION_REASON_INITIALIZATION);
  EXPECT_FALSE(fake_enrollment_manager_->GetAndClearInvocationReason());
}

TEST_F(DeviceSyncImpltest, TestInvalidEnrollmentOnInit) {
  EXPECT_FALSE(fake_device_manager_->GetAndClearInvocationReason());
  EXPECT_FALSE(fake_enrollment_manager_->GetAndClearInvocationReason());
  fake_enrollment_manager_->SetIsEnrollmentValid(false);
  CreateDeviceSync();
  EXPECT_EQ(fake_device_manager_->GetAndClearInvocationReason(),
            InvocationReason::INVOCATION_REASON_INITIALIZATION);
  EXPECT_EQ(fake_enrollment_manager_->GetAndClearInvocationReason(),
            InvocationReason::INVOCATION_REASON_INITIALIZATION);
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

TEST_F(DeviceSyncImpltest, TestGetSyncedDevices) {
  fake_remote_device_provider_->set_synced_remote_devices(
      cryptauth::GenerateTestRemoteDevices(5));
  CreateDeviceSync();
  device_sync_->GetSyncedDevices(base::Bind(
      &DeviceSyncImpltest::GetSyncedDevicesCallback, base::Unretained(this)));
  fake_remote_device_provider_->set_synced_remote_devices(
      cryptauth::GenerateTestRemoteDevices(3));
  device_sync_->GetSyncedDevices(base::Bind(
      &DeviceSyncImpltest::GetSyncedDevicesCallback, base::Unretained(this)));
}

TEST_F(DeviceSyncImpltest, TestAddObserver) {
  LOG(ERROR) << "DEVICE SYNC OBSERVER PTR";
  device_sync::mojom::DeviceSyncObserverPtr device_sync_observer_ptr =
      device_sync::mojom::DeviceSyncObserverPtr();
  device_sync_observer_ = base::MakeUnique<DeviceSyncObserverImpl>(
      mojo::MakeRequest(&device_sync_observer_ptr));
  device_sync_->AddObserver(std::move(device_sync_observer_ptr));
  LOG(ERROR) << "DEVICE SYNC OBSERVER PTR 2";
}

}  // namespace multidevice