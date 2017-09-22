// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/device_sync_impl.h"

#include "base/barrier_closure.h"
#include "base/base64.h"
#include "base/memory/ptr_util.h"
#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/cryptauth_enroller.h"
#include "components/cryptauth/cryptauth_enrollment_manager.h"
#include "components/cryptauth/device_capability_manager_impl.h"
#include "components/cryptauth/fake_cryptauth_gcm_manager.h"
#include "components/cryptauth/fake_device_capability_manager.h"
#include "components/cryptauth/fake_remote_device_provider.h"
#include "components/cryptauth/fake_secure_message_delegate.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device_provider_impl.h"
#include "components/cryptauth/remote_device_test_util.h"
#include "components/multidevice/service/device_sync_mojom_result_code_util.h"
#include "components/signin/core/browser/account_info.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
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
using DeviceChangeResult =
    cryptauth::CryptAuthDeviceManager::DeviceChangeResult;
using InvocationReason = cryptauth::InvocationReason;
using Capability = cryptauth::DeviceCapabilityManager::Capability;
using SetCapabilityResponse = device_sync::mojom::SetCapabilityResponse;

namespace multidevice {

namespace {
const std::string kRegistrationId = "registrationId";
const std::string kDeviceId = "deviceId";
const std::string kAccountId = "accountId";
const std::string kUserPrivateKey = "userPrivateKey";
const std::string kUserPublicKey = "userPublicKey";
const int kNumObservers = 3;

enum class DeviceSyncActionType { FORCE_ENROLLMENT_NOW, FORCE_SYNC_NOW };

// Used for ForceManagerAction to initiate an enrollment or device sync, and
// also with what parameters observers should be notified.
struct ForcedManagerActionParams {
  // DeviceSyncActionType is FORCE_ENROLLMENT_NOW
  ForcedManagerActionParams(bool enrollment_success)
      : type(DeviceSyncActionType::FORCE_ENROLLMENT_NOW),
        last_enrollment_attempt(enrollment_success){};

  // DeviceSyncActionType is FORCE_SYNC_NOW
  ForcedManagerActionParams(SyncResult synced, DeviceChangeResult changed)
      : type(DeviceSyncActionType::FORCE_SYNC_NOW),
        last_sync_result(synced),
        last_change_result(changed) {}

  DeviceSyncActionType type;

  bool last_enrollment_attempt = false;

  SyncResult last_sync_result = SyncResult::SUCCESS;
  DeviceChangeResult last_change_result = DeviceChangeResult::CHANGED;
};

std::vector<cryptauth::ExternalDeviceInfo>
CreateExternalDeviceInfosForRemoteDevices(int num) {
  std::vector<cryptauth::ExternalDeviceInfo> device_infos;
  for (int i = 0; i < num; i++) {
    cryptauth::ExternalDeviceInfo info;
    info.set_public_key("publicKey" + std::to_string(i));
    device_infos.push_back(info);
  }
  return device_infos;
}

std::vector<cryptauth::IneligibleDevice>
CreateIneligibleDeviceInfosForExternalDeviceInfos(int num) {
  std::vector<cryptauth::IneligibleDevice> ineligible_devices;
  for (int i = 0; i < num; i++) {
    ineligible_devices.emplace_back(cryptauth::IneligibleDevice());
    ineligible_devices[i].set_allocated_device(
        new cryptauth::ExternalDeviceInfo());
    ineligible_devices[i].mutable_device()->set_public_key("publicKey" +
                                                           std::to_string(i));
    ineligible_devices[i].add_reasons("reason" + std::to_string(i));
  }
  return ineligible_devices;
}

std::string GetDeviceID(const std::string public_key) {
  std::string device_id;
  base::Base64Encode(public_key, &device_id);
  return device_id;
}

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

// Stores invocation reasons on behalf of other classes and resets
// the invocation reason when tests retrieve the invocation reason.
class FakeManagerBase {
 public:
  ~FakeManagerBase() {}

  InvocationReason GetAndClearInvocationReason() {
    InvocationReason invocation_reason = expected_invocation_reason_;
    expected_invocation_reason_ = InvocationReason::INVOCATION_REASON_UNKNOWN;
    return invocation_reason;
  }

  void set_expected_invocation_reason(InvocationReason invocation_reason) {
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

  void NotifyEnrollmentFinished(bool successful_enrollment) {
    cryptauth::CryptAuthEnrollmentManager::NotifyEnrollmentFinished(
        successful_enrollment);
  }

  void set_is_enrollment_valid(bool status) { is_enrollment_valid_ = status; }

  // cryptauth::CryptAuthEnrollmentManager:
  std::string GetUserPrivateKey() const override { return user_private_key_; }

  bool IsEnrollmentValid() const override { return is_enrollment_valid_; }

  void ForceEnrollmentNow(InvocationReason invocation_reason) override {
    set_expected_invocation_reason(invocation_reason);
  }

  void set_user_public_key(std::string public_key) {
    user_public_key_ = public_key;
  };

  std::string GetUserPublicKey() const override { return user_public_key_; }

 private:
  std::string user_public_key_ = kUserPublicKey;
  std::string user_private_key_ = kUserPrivateKey;
  bool is_enrollment_valid_ = true;
};

class FakeCryptAuthDeviceManager : public cryptauth::CryptAuthDeviceManager,
                                   public FakeManagerBase {
 public:
  FakeCryptAuthDeviceManager() {}
  ~FakeCryptAuthDeviceManager() override {}

  void NotifySyncFinished(SyncResult expected_sync_result,
                          DeviceChangeResult expected_device_change_result) {
    cryptauth::CryptAuthDeviceManager::NotifySyncFinished(
        expected_sync_result, expected_device_change_result);
  }

  // cryptauth::CryptAuthDeviceManager:
  void ForceSyncNow(InvocationReason invocation_reason) override {
    set_expected_invocation_reason(invocation_reason);
  }
};

class FakeIdentityManager : public identity::mojom::IdentityManager {
 public:
  FakeIdentityManager() {}
  ~FakeIdentityManager() override {}

  void InvokeAccountAvailableCallback() {
    DCHECK(callback_);
    AccountInfo info;
    info.account_id = kAccountId;
    std::move(callback_).Run(info, identity::AccountState());
  }

  // identity::mojom::IdentityManager:
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
    callback_ = std::move(callback);
  }

 private:
  GetPrimaryAccountWhenAvailableCallback callback_;
};

class FakeDeviceCapabilityManagerFactory final
    : public cryptauth::DeviceCapabilityManagerImpl::Factory {
 public:
  FakeDeviceCapabilityManagerFactory(){};
  std::unique_ptr<cryptauth::DeviceCapabilityManager> BuildInstance(
      cryptauth::CryptAuthClientFactory* cryptauth_client_factory) override {
    last_instance_ = new cryptauth::FakeDeviceCapabilityManager();
    return base::WrapUnique(last_instance_);
  }

  cryptauth::FakeDeviceCapabilityManager* last_instance() {
    return last_instance_;
  }

 private:
  cryptauth::FakeDeviceCapabilityManager* last_instance_ = nullptr;
};

class FakeRemoteDeviceProviderFactory final
    : public cryptauth::RemoteDeviceProviderImpl::Factory {
 public:
  FakeRemoteDeviceProviderFactory() {}

  std::unique_ptr<cryptauth::RemoteDeviceProvider> BuildInstance(
      cryptauth::CryptAuthDeviceManager* device_manager,
      const std::string& user_id,
      const std::string& user_private_key,
      cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory)
      override {
    last_instance_ = new cryptauth::FakeRemoteDeviceProvider();
    return base::WrapUnique(last_instance_);
  }

  cryptauth::FakeRemoteDeviceProvider* last_instance() {
    return last_instance_;
  }

 private:
  cryptauth::FakeRemoteDeviceProvider* last_instance_ = nullptr;
};

class DeviceSyncImpltest : public service_manager::test::ServiceTest {
 public:
  class DeviceSyncObserverImpl : public device_sync::mojom::DeviceSyncObserver {
   public:
    DeviceSyncObserverImpl(
        device_sync::mojom::DeviceSyncObserverRequest request)
        : binding_(this, std::move(request)) {}

    void OnEnrollmentFinished(bool success) override {
      if (success)
        num_times_enrollment_finished_called_.success_count++;
      else
        num_times_enrollment_finished_called_.failure_count++;
      on_callback_invoked_->Run();
    }

    void OnDevicesSynced(bool success, bool device_list_changed) override {
      if (success && device_list_changed)
        num_times_device_synced_.success_count++;
      else
        num_times_device_synced_.failure_count++;
      on_callback_invoked_->Run();
    }

    // Sets the necessary callback that will be invoked upon each interface
    // method call in order to return control to the test.
    void SetOnCallbackInvokedClosure(base::Closure* on_callback_invoked) {
      on_callback_invoked_ = on_callback_invoked;
    }

    int GetNumCallsEnrollment(bool success_count) {
      return num_times_enrollment_finished_called_.CountType(success_count);
    }

    int GetNumCallsSync(bool success_count) {
      return num_times_device_synced_.CountType(success_count);
    }

    void ClearNotificationCount() {
      num_times_enrollment_finished_called_.Reset();
      num_times_device_synced_.Reset();
    }

   private:
    struct ObserverCallbackCount {
      int success_count = 0;
      int failure_count = 0;
      int CountType(bool success) {
        return success ? success_count : failure_count;
      }
      void Reset() {
        success_count = 0;
        failure_count = 0;
      }
    };

    base::Closure* on_callback_invoked_ = nullptr;

    mojo::Binding<device_sync::mojom::DeviceSyncObserver> binding_;

    ObserverCallbackCount num_times_enrollment_finished_called_;
    ObserverCallbackCount num_times_device_synced_;
  };

  DeviceSyncImpltest() {}

  void SetUp() override {
    fake_secure_message_delegate_factory_ =
        base::MakeUnique<FakeSecureMessageDelegateFactory>();

    fake_gcm_manager_ =
        base::MakeUnique<cryptauth::FakeCryptAuthGCMManager>(kRegistrationId);

    fake_identity_manager_ = base::WrapUnique(new FakeIdentityManager());

    fake_enrollment_manager_ = base::WrapUnique(
        new FakeCryptAuthEnrollmentManager(fake_gcm_manager_.get()));

    fake_device_manager_ = base::WrapUnique(new FakeCryptAuthDeviceManager());

    fake_remote_device_factory_ =
        base::WrapUnique(new FakeRemoteDeviceProviderFactory());
    cryptauth::RemoteDeviceProviderImpl::Factory::SetInstanceForTesting(
        fake_remote_device_factory_.get());

    fake_remote_device_provider_ =
        base::WrapUnique(new cryptauth::FakeRemoteDeviceProvider());

    fake_device_capability_manager_factory_ =
        base::WrapUnique(new FakeDeviceCapabilityManagerFactory());
    cryptauth::DeviceCapabilityManagerImpl::Factory::SetInstanceForTesting(
        fake_device_capability_manager_factory_.get());

    fake_device_capability_manager_ =
        base::WrapUnique(new cryptauth::FakeDeviceCapabilityManager());
  }

  void TearDown() override {}

  void CreateDeviceSyncAndAddObservers() {
    device_sync_ = base::MakeUnique<DeviceSyncImpl>(
        nullptr,  // what should I put here?????

        fake_identity_manager_.get(), fake_gcm_manager_.get(),
        fake_device_manager_.get(), fake_enrollment_manager_.get(),
        fake_secure_message_delegate_factory_.get(),
        cryptauth::GcmDeviceInfo());

    AddDeviceSyncObservers(kNumObservers);
  }

  void VerifyNoEnrollmentsSinceLastCheck() {
    EXPECT_EQ(fake_enrollment_manager_->GetAndClearInvocationReason(),
              InvocationReason::INVOCATION_REASON_UNKNOWN);
  }

  void VerifyNoSyncsSinceLastCheck() {
    EXPECT_EQ(fake_device_manager_->GetAndClearInvocationReason(),
              InvocationReason::INVOCATION_REASON_UNKNOWN);
  }

  void VerifySyncedDevices(
      const std::vector<cryptauth::RemoteDevice>& expected_synced_devices,
      const std::vector<cryptauth::RemoteDevice>& actual_synced_devices) {
    EXPECT_EQ(expected_synced_devices, actual_synced_devices);
  }

  void SetCapabilityEnabledCallback(
      device_sync::mojom::ResultCode expected_result_code,
      device_sync::mojom::SetCapabilityResponsePtr response) {
    EXPECT_EQ(expected_result_code, response->result_code);
  }

  void FindEligibleDevicesForCapabilityCallback(
      device_sync::mojom::ResultCode expected_result_code,
      const std::vector<cryptauth::ExternalDeviceInfo>&
          expected_eligible_devices,
      const std::vector<cryptauth::IneligibleDevice>&
          expected_ineligible_devices,
      device_sync::mojom::FindEligibleDevicesResponsePtr response) {
    EXPECT_EQ(expected_result_code, response->result_code);
    std::vector<std::string> expected_ids;
    for (const auto& eligible_device : expected_eligible_devices) {
      expected_ids.emplace_back(GetDeviceID(eligible_device.public_key()));
    }
    EXPECT_EQ(expected_ids, response->eligible_device_ids);
    EXPECT_EQ(expected_ineligible_devices.size(),
              response->ineligible_devices.size());

    for (int i = 0; i < fmin(expected_ineligible_devices.size(),
                             response->ineligible_devices.size());
         i++) {
      for (int j = 0;
           j < fmin(expected_ineligible_devices[i].reasons_size(),
                    (response->ineligible_devices)[i].reasons_size());
           j++) {
        EXPECT_EQ(expected_ineligible_devices[i].reasons(j),
                  (response->ineligible_devices)[i].reasons(j));
      }
    }
  }

  void IsCapabilityPromotableCallback(
      bool is_promotable,
      device_sync::mojom::ResultCode expected_result_code,

      device_sync::mojom::IsCapabilityPromotableResponsePtr response) {
    EXPECT_EQ(is_promotable, response->is_promotable);
    EXPECT_EQ(expected_result_code, response->result_code);
  }

  void GetUserPublicKeyCallback(const std::string& expected_public_key,
                                const std::string& actual_public_key) {
    EXPECT_EQ(actual_public_key, expected_public_key);
  }

  void AddDeviceSyncObservers(int num) {
    device_sync::mojom::DeviceSyncObserverPtr device_sync_observer_ptr;
    for (int i = 0; i < num; i++) {
      device_sync_observer_ptr = device_sync::mojom::DeviceSyncObserverPtr();
      observers_.emplace_back(base::MakeUnique<DeviceSyncObserverImpl>(
          mojo::MakeRequest(&device_sync_observer_ptr)));
      device_sync_->AddObserver(std::move(device_sync_observer_ptr));
    }
  }

  void ConfirmObserversNotified(DeviceSyncActionType action_type,
                                int num_expected_success_callbacks,
                                int num_expected_error_callbacks) {
    for (auto& observer : observers_) {
      if (action_type == DeviceSyncActionType::FORCE_ENROLLMENT_NOW) {
        EXPECT_EQ(observer->GetNumCallsEnrollment(true /*success_count*/),
                  num_expected_success_callbacks);
        EXPECT_EQ(observer->GetNumCallsEnrollment(false /*success_count*/),
                  num_expected_error_callbacks);
        continue;
      }
      EXPECT_EQ(observer->GetNumCallsSync(true /*success_count*/),
                num_expected_success_callbacks);
      EXPECT_EQ(observer->GetNumCallsSync(false /*success_count*/),
                num_expected_error_callbacks);
    }
  }

  void ClearAllObserverNotificationCounts() {
    for (auto& observer : observers_) {
      observer->ClearNotificationCount();
    }
  }

  void ForceManagerAction(ForcedManagerActionParams instructions) {
    base::RunLoop run_loop;
    base::Closure closure = base::BarrierClosure(
        static_cast<int>(observers_.size()), run_loop.QuitClosure());
    for (auto& observer : observers_) {
      observer->SetOnCallbackInvokedClosure(&closure);
    }

    switch (instructions.type) {
      case DeviceSyncActionType::FORCE_ENROLLMENT_NOW:
        device_sync_->ForceEnrollmentNow();
        EXPECT_EQ(InvocationReason::INVOCATION_REASON_MANUAL,
                  fake_enrollment_manager_->GetAndClearInvocationReason());
        fake_enrollment_manager_->NotifyEnrollmentFinished(
            instructions.last_enrollment_attempt);
        break;
      case DeviceSyncActionType::FORCE_SYNC_NOW:
        device_sync_->ForceSyncNow();
        EXPECT_EQ(InvocationReason::INVOCATION_REASON_MANUAL,
                  fake_device_manager_->GetAndClearInvocationReason());
        fake_device_manager_->NotifySyncFinished(
            instructions.last_sync_result, instructions.last_change_result);
        break;
      default:
        NOTREACHED();
    }

    run_loop.Run();
  }

  void NotifyObserverDeviceListChange() {
    base::RunLoop run_loop;
    base::Closure closure = base::BarrierClosure(
        static_cast<int>(observers_.size()), run_loop.QuitClosure());
    for (auto& observer : observers_) {
      observer->SetOnCallbackInvokedClosure(&closure);
    }
    fake_remote_device_factory_->last_instance()
        ->NotifyObserversDeviceListChanged();
    run_loop.Run();
  }

  std::unique_ptr<cryptauth::FakeCryptAuthGCMManager> fake_gcm_manager_;

  std::unique_ptr<FakeSecureMessageDelegateFactory>
      fake_secure_message_delegate_factory_;

  std::unique_ptr<FakeIdentityManager> fake_identity_manager_;

  std::unique_ptr<FakeCryptAuthEnrollmentManager> fake_enrollment_manager_;

  std::unique_ptr<FakeRemoteDeviceProviderFactory> fake_remote_device_factory_;

  std::unique_ptr<cryptauth::FakeRemoteDeviceProvider>
      fake_remote_device_provider_;

  std::unique_ptr<FakeCryptAuthDeviceManager> fake_device_manager_;

  std::unique_ptr<FakeDeviceCapabilityManagerFactory>
      fake_device_capability_manager_factory_;

  std::unique_ptr<cryptauth::FakeDeviceCapabilityManager>
      fake_device_capability_manager_;

  std::unique_ptr<DeviceSyncImpl> device_sync_;

  std::vector<std::unique_ptr<DeviceSyncObserverImpl>> observers_;

  device_sync::mojom::DeviceSyncPtr device_sync_ptr_;
};

TEST_F(DeviceSyncImpltest, TestValidEnrollmentOnInit) {
  fake_enrollment_manager_->set_is_enrollment_valid(true);
  CreateDeviceSyncAndAddObservers();
  VerifyNoEnrollmentsSinceLastCheck();
  VerifyNoSyncsSinceLastCheck();

  fake_identity_manager_->InvokeAccountAvailableCallback();

  // Enrollment is already valid, so enrollment does not occur.
  VerifyNoEnrollmentsSinceLastCheck();

  EXPECT_EQ(fake_device_manager_->GetAndClearInvocationReason(),
            InvocationReason::INVOCATION_REASON_INITIALIZATION);
}

TEST_F(DeviceSyncImpltest, TestInvalidEnrollmentOnInit) {
  fake_enrollment_manager_->set_is_enrollment_valid(false);
  CreateDeviceSyncAndAddObservers();
  VerifyNoEnrollmentsSinceLastCheck();
  VerifyNoSyncsSinceLastCheck();

  fake_identity_manager_->InvokeAccountAvailableCallback();

  // Enrollment is invalid, so enrollment occurs.
  EXPECT_EQ(fake_enrollment_manager_->GetAndClearInvocationReason(),
            InvocationReason::INVOCATION_REASON_INITIALIZATION);
  fake_enrollment_manager_->NotifyEnrollmentFinished(true);
  EXPECT_EQ(fake_device_manager_->GetAndClearInvocationReason(),
            InvocationReason::INVOCATION_REASON_INITIALIZATION);
}

TEST_F(DeviceSyncImpltest, TestForceEnrollment) {
  CreateDeviceSyncAndAddObservers();
  fake_identity_manager_->InvokeAccountAvailableCallback();

  // Verify that when ForceEnrollmentNow() is successful, observers are
  // notified.
  ForceManagerAction(ForcedManagerActionParams(true /* enrollment_success */));
  ConfirmObserversNotified(DeviceSyncActionType::FORCE_ENROLLMENT_NOW,
                           1 /* num_expected_success_callbacks */,
                           0 /* num_expected_error_callbacks */);

  // Verify that when ForceEnrollmentNow() fails, observers are notified.
  ForceManagerAction(ForcedManagerActionParams(false /* enrollment_success */));
  ConfirmObserversNotified(DeviceSyncActionType::FORCE_ENROLLMENT_NOW,
                           1 /* num_expected_success_callbacks */,
                           1 /* num_expected_error_callbacks */);
}

TEST_F(DeviceSyncImpltest, TestForceSync) {
  CreateDeviceSyncAndAddObservers();
  fake_identity_manager_->InvokeAccountAvailableCallback();

  // Verify that when ForceSyncNow() is successful, observers are notified.
  ForceManagerAction(ForcedManagerActionParams(SyncResult::SUCCESS,
                                               DeviceChangeResult::CHANGED));
  ConfirmObserversNotified(DeviceSyncActionType::FORCE_SYNC_NOW,
                           1 /* num_expected_success_callbacks */,
                           0 /* num_expected_error_callbacks */);

  // Verify that when ForceSyncNow() fails, observers are notified.
  ForceManagerAction(ForcedManagerActionParams(SyncResult::FAILURE,
                                               DeviceChangeResult::CHANGED));
  ConfirmObserversNotified(DeviceSyncActionType::FORCE_SYNC_NOW,
                           1 /* num_expected_success_callbacks */,
                           1 /* num_expected_error_callbacks */);

  ForceManagerAction(ForcedManagerActionParams(SyncResult::FAILURE,
                                               DeviceChangeResult::UNCHANGED));
  ConfirmObserversNotified(DeviceSyncActionType::FORCE_SYNC_NOW,
                           1 /* num_expected_success_callbacks */,
                           2 /* num_expected_error_callbacks */);

  ForceManagerAction(ForcedManagerActionParams(SyncResult::SUCCESS,
                                               DeviceChangeResult::UNCHANGED));
  ConfirmObserversNotified(DeviceSyncActionType::FORCE_SYNC_NOW,
                           1 /* num_expected_success_callbacks */,
                           3 /* num_expected_error_callbacks */);

  // Verify that ForceSyncNow() can still succeed after fail(s).
  ForceManagerAction(ForcedManagerActionParams(SyncResult::SUCCESS,
                                               DeviceChangeResult::CHANGED));
  ConfirmObserversNotified(DeviceSyncActionType::FORCE_SYNC_NOW,
                           2 /* num_expected_success_callbacks */,
                           3 /* num_expected_error_callbacks */);
}

TEST_F(DeviceSyncImpltest, TestGetSyncedDevices) {
  CreateDeviceSyncAndAddObservers();
  fake_identity_manager_->InvokeAccountAvailableCallback();

  ConfirmObserversNotified(DeviceSyncActionType::FORCE_SYNC_NOW,
                           0 /* num_expected_success_callbacks */,
                           0 /* num_expected_error_callbacks */);

  std::vector<cryptauth::RemoteDevice> first_list_of_devices =
      cryptauth::GenerateTestRemoteDevices(5);
  fake_remote_device_factory_->last_instance()->set_synced_remote_devices(
      first_list_of_devices);

  NotifyObserverDeviceListChange();
  ConfirmObserversNotified(DeviceSyncActionType::FORCE_SYNC_NOW,
                           1 /* num_expected_success_callbacks */,
                           0 /* num_expected_error_callbacks */);

  // Verify that GetSyncedDevices() returns first_list_of_devices.
  device_sync_->GetSyncedDevices(
      base::Bind(&DeviceSyncImpltest::VerifySyncedDevices,
                 base::Unretained(this), first_list_of_devices));

  std::vector<cryptauth::RemoteDevice> second_list_of_devices =
      cryptauth::GenerateTestRemoteDevices(10);
  fake_remote_device_factory_->last_instance()->set_synced_remote_devices(
      second_list_of_devices);

  NotifyObserverDeviceListChange();
  ConfirmObserversNotified(DeviceSyncActionType::FORCE_SYNC_NOW,
                           2 /* num_expected_success_callbacks */,
                           0 /* num_expected_error_callbacks */);

  // Verify that GetSyncedDevices() returns second_list_of_devices.
  device_sync_->GetSyncedDevices(
      base::Bind(&DeviceSyncImpltest::VerifySyncedDevices,
                 base::Unretained(this), second_list_of_devices));
}

TEST_F(DeviceSyncImpltest, TestGetUserPublicKey) {
  CreateDeviceSyncAndAddObservers();
  fake_identity_manager_->InvokeAccountAvailableCallback();

  // Verify that GetUserPublicKey() returns the expected public key.
  std::string expected_pk = kUserPublicKey;
  fake_enrollment_manager_->set_user_public_key(expected_pk);
  device_sync_->GetUserPublicKey(
      base::Bind(&DeviceSyncImpltest::GetUserPublicKeyCallback,
                 base::Unretained(this), expected_pk));
}

TEST_F(DeviceSyncImpltest, TestSetCapabilityEnabled) {
  CreateDeviceSyncAndAddObservers();
  fake_identity_manager_->InvokeAccountAvailableCallback();

  // Verify that SetCapabilityEnabled() returns SUCCESS on success.
  fake_device_capability_manager_factory_->last_instance()
      ->set_capability_enabled_error_code("" /* error_code */);
  device_sync::mojom::ResultCode expected_result_code =
      ConvertToMojomResultCode(kSuccessCode);
  device_sync_->SetCapabilityEnabled(
      kDeviceId, Capability::CAPABILITY_UNLOCK_KEY, true,
      base::BindOnce(&DeviceSyncImpltest::SetCapabilityEnabledCallback,
                     base::Unretained(this), expected_result_code));

  // Verify that SetCapabilityEnabled() returns ERROR on error.
  fake_device_capability_manager_factory_->last_instance()
      ->set_capability_enabled_error_code(kErrorInternal);
  expected_result_code = ConvertToMojomResultCode(kErrorInternal);
  device_sync_->SetCapabilityEnabled(
      kDeviceId, Capability::CAPABILITY_UNLOCK_KEY, true,
      base::BindOnce(&DeviceSyncImpltest::SetCapabilityEnabledCallback,
                     base::Unretained(this), expected_result_code));
}

TEST_F(DeviceSyncImpltest, TestFindEligibleDevicesForCapability) {
  CreateDeviceSyncAndAddObservers();
  fake_identity_manager_->InvokeAccountAvailableCallback();

  std::vector<cryptauth::ExternalDeviceInfo> expected_external_device_info =
      CreateExternalDeviceInfosForRemoteDevices(5);
  std::vector<cryptauth::IneligibleDevice> expected_ineligible_devices =
      CreateIneligibleDeviceInfosForExternalDeviceInfos(3);

  // Verify that FindEligibleDevicesForCapability() returns the expected devices
  // on SUCCESS.
  fake_device_capability_manager_factory_->last_instance()
      ->set_find_eligible_devices_for_capability_error_code(
          "" /* error_code */);
  fake_device_capability_manager_factory_->last_instance()
      ->set_external_device_info(expected_external_device_info);
  fake_device_capability_manager_factory_->last_instance()
      ->set_ineligible_devices(expected_ineligible_devices);
  device_sync::mojom::ResultCode expected_result_code =
      ConvertToMojomResultCode(kSuccessCode);

  device_sync_->FindEligibleDevicesForCapability(
      Capability::CAPABILITY_UNLOCK_KEY,
      base::BindOnce(
          &DeviceSyncImpltest::FindEligibleDevicesForCapabilityCallback,
          base::Unretained(this), expected_result_code,
          expected_external_device_info, expected_ineligible_devices));

  // Verify that FindEligibleDevicesForCapability() returns no devices on
  // failure.
  fake_device_capability_manager_factory_->last_instance()
      ->set_find_eligible_devices_for_capability_error_code(kErrorInternal);
  expected_result_code = ConvertToMojomResultCode(kErrorInternal);

  device_sync_->FindEligibleDevicesForCapability(
      Capability::CAPABILITY_UNLOCK_KEY,
      base::BindOnce(
          &DeviceSyncImpltest::FindEligibleDevicesForCapabilityCallback,
          base::Unretained(this), expected_result_code,
          std::vector<cryptauth::ExternalDeviceInfo>(),
          std::vector<cryptauth::IneligibleDevice>()));
}

TEST_F(DeviceSyncImpltest, TestIsCapabilityPromotable) {
  CreateDeviceSyncAndAddObservers();
  fake_identity_manager_->InvokeAccountAvailableCallback();

  // Verify that IsCapabilityPromotable() a promotable capability returns
  // success code.
  device_sync::mojom::ResultCode expected_result_code =
      ConvertToMojomResultCode(kSuccessCode);
  fake_device_capability_manager_factory_->last_instance()
      ->set_is_capability_promotable_error_code("" /* error_code */);
  device_sync_->IsCapabilityPromotable(
      kDeviceId, Capability::CAPABILITY_UNLOCK_KEY,
      base::BindOnce(&DeviceSyncImpltest::IsCapabilityPromotableCallback,
                     base::Unretained(this), true /* is_promotable */,
                     expected_result_code));

  // Verify that a non-promotable capability returns error code.
  expected_result_code = ConvertToMojomResultCode(kErrorInternal);
  fake_device_capability_manager_factory_->last_instance()
      ->set_is_capability_promotable_error_code(kErrorInternal);
  device_sync_->IsCapabilityPromotable(
      kDeviceId, Capability::CAPABILITY_UNLOCK_KEY,
      base::BindOnce(&DeviceSyncImpltest::IsCapabilityPromotableCallback,
                     base::Unretained(this), false /* is_promotable */,
                     expected_result_code));
}

}  // namespace multidevice
