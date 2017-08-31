// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/device_sync_impl.h"

#include "chrome/browser/chromeos/login/easy_unlock/secure_message_delegate_chromeos.h"
#include "components/cryptauth/proto/cryptauth_api.pb.h"
#include "components/cryptauth/remote_device_provider.h"
#include "services/identity/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace multidevice {

DeviceSyncImpl::DeviceSyncImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref,
    service_manager::Connector* connector,
    cryptauth::CryptAuthGCMManager* gcm_manager,
    cryptauth::CryptAuthDeviceManager* device_manager,
    cryptauth::CryptAuthEnrollmentManager*
        enrollment_manager,
    cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory)
    : service_ref_(std::move(service_ref)),
      connector_(connector),
      gcm_manager_(gcm_manager),
      enrollment_manager_(enrollment_manager),
      device_manager_(device_manager),
      secure_message_delegate_factory_(secure_message_delegate_factory),
      weak_ptr_factory_(this) {
  gcm_manager_->StartListening();
  enrollment_manager_->AddObserver(this);
  device_manager_->AddObserver(this);

  connector_->BindInterface(identity::mojom::kServiceName,
                            mojo::MakeRequest(&identity_manager_));

  // RemoteDeviceProvider can only be constructed when the private key is
  // generated.  Private key can only be generated when primary account info is
  // available.  On completing RemoteDeviceProvider construction, call a force
  // enrollment and sync.
  GetAndSetState(State::WAITING_FOR_PRIMARY_ACCOUNT);
  identity_manager_->GetPrimaryAccountWhenAvailable(
      OnPrimaryAccountAvailableCallback(
          GenerateKeyPairCallback(
            ConstructRemoteDeviceProviderCallback(
              ForceEnrollmentNowCallback()))));
}

DeviceSyncImpl::~DeviceSyncImpl() {
  enrollment_manager_->RemoveObserver(this);
  device_manager_->RemoveObserver(this);
}

void DeviceSyncImpl::ForceEnrollmentNow() {
  cryptauth::InvocationReason reason;
  switch(GetAndSetState(State::WAITING_FOR_ACCOUNT_ENROLLMENT)) {
    case State::NOTIFY_REMOTE_DEVICE_PROVIDER_INITIALIZED:
      reason = cryptauth::InvocationReason::INVOCATION_REASON_INITIALIZATION;
      //TODO(hsuregan): Or, enrollment_manager_->Start();
      break;
    default:
      reason = cryptauth::InvocationReason::INVOCATION_REASON_UNKNOWN;
      break;
  }
  enrollment_manager_->ForceEnrollmentNow(reason);
}

void DeviceSyncImpl::ForceSyncNow() {
  cryptauth::InvocationReason reason;
  switch(GetAndSetState(State::WAITING_FOR_DEVICE_SYNC)) {
    case State::NOTIFY_ENROLLMENT_FINISHED:
      reason = cryptauth::InvocationReason::INVOCATION_REASON_INITIALIZATION;
      //TODO(hsuregan): Or, device_manager_->Start();
      break;
    default:
      reason = cryptauth::InvocationReason::INVOCATION_REASON_UNKNOWN;
      break;
  }
  device_manager_->ForceSyncNow(reason);
}

void DeviceSyncImpl::GetSyncedDevices(GetSyncedDevicesCallback callback) {
  std::move(callback).Run(remote_device_provider_->GetSyncedDevices());
}

void DeviceSyncImpl::AddObserver(
    device_sync::mojom::DeviceSyncObserverPtr observer) {
  observers_.AddPtr(std::move(observer));
}

void DeviceSyncImpl::OnEnrollmentStarted() {}

void DeviceSyncImpl::OnEnrollmentFinished(bool success) {
  GetAndSetState(State::NOTIFY_ENROLLMENT_FINISHED);
  observers_.ForAllPtrs(
      [&success](device_sync::mojom::DeviceSyncObserver* observer) {
        observer->OnEnrollmentFinished(success /* success */);
      });
  if (success) {
    ForceSyncNow();
  }
}

void DeviceSyncImpl::OnSyncStarted() {}

void DeviceSyncImpl::OnSyncFinished(
    cryptauth::CryptAuthDeviceManager::SyncResult sync_result,
    cryptauth::CryptAuthDeviceManager::DeviceChangeResult
        device_change_result) {
  GetAndSetState(State::NOTIFY_SYNC_FINISHED);
  observers_.ForAllPtrs([&sync_result, &device_change_result](
                            device_sync::mojom::DeviceSyncObserver* observer) {
    observer->OnDevicesSynced(
        sync_result == cryptauth::CryptAuthDeviceManager::SyncResult::
                           SUCCESS /* success */,
        device_change_result ==
            cryptauth::CryptAuthDeviceManager::DeviceChangeResult::
                CHANGED /* device_change_result*/);
  });
}

void DeviceSyncImpl::GenerateKeyPair(base::Closure OnKeyPairGenerated) {
  GetAndSetState(State::WAITING_FOR_KEY_PAIR_GENERATION);
  secure_message_delegate_factory_->CreateSecureMessageDelegate()->GenerateKeyPair(
      base::Bind(&DeviceSyncImpl::OnKeyPairGenerated,
                 weak_ptr_factory_.GetWeakPtr(), OnKeyPairGenerated));
}

DeviceSyncImpl::State DeviceSyncImpl::GetAndSetState(State new_state) {
  std::swap(new_state, current_state_);
  return new_state;
}

void DeviceSyncImpl::ContructRemoteDeviceProvider(
    base::Closure OnRemoteDeviceProviderConstructed) {
  if (primary_account_info_.has_value()) {
    DCHECK(primary_account_info_->IsValid());
    DCHECK(!private_key_.empty());
    remote_device_provider_ = base::MakeUnique<cryptauth::RemoteDeviceProvider>(
        device_manager_.get(), primary_account_info_->account_id, private_key_,
        secure_message_delegate_factory_.get());
    GetAndSetState(State::NOTIFY_REMOTE_DEVICE_PROVIDER_INITIALIZED);
    OnRemoteDeviceProviderConstructed.Run();
  }
}

void DeviceSyncImpl::OnPrimaryAccountAvailable(
    base::Closure OnAccountInfoStored,
    const AccountInfo& account_info,
    const identity::AccountState& account_state) {
  GetAndSetState(State::NOTIFY_PRIMARY_ACCOUNT_AVAILABLE);
  primary_account_info_ = account_info;
  primary_account_state_ = account_state;
  OnAccountInfoStored.Run();
}

void DeviceSyncImpl::OnKeyPairGenerated(base::Closure OnKeyPairStored,
                                        const std::string& public_key,
                                        const std::string& private_key) {
  GetAndSetState(State::NOTIFY_KEY_PAIR_GENERATED);
  public_key_ = public_key;
  private_key_ = private_key;
  OnKeyPairStored.Run();
}

base::Callback<void(const AccountInfo& account_info,
                    const identity::AccountState& account_state)>
DeviceSyncImpl::OnPrimaryAccountAvailableCallback(
    base::Closure OnAccountInfoStored) {
  return  base::Bind(&DeviceSyncImpl::OnPrimaryAccountAvailable,
    weak_ptr_factory_.GetWeakPtr(), OnAccountInfoStored);
 }

base::Closure DeviceSyncImpl::GenerateKeyPairCallback(base::Closure OnKeyPairGenerated) {
  return base::Bind(&DeviceSyncImpl::GenerateKeyPair, weak_ptr_factory_.GetWeakPtr(), OnKeyPairGenerated);
}

base::Closure DeviceSyncImpl::ConstructRemoteDeviceProviderCallback(base::Closure OnRemoteDeviceProviderConstructed) {
  return base::Bind(&DeviceSyncImpl::ContructRemoteDeviceProvider, weak_ptr_factory_.GetWeakPtr(), OnRemoteDeviceProviderConstructed);
}

base::Closure DeviceSyncImpl::ForceEnrollmentNowCallback() {
  return base::Bind(&DeviceSyncImpl::ForceEnrollmentNow, weak_ptr_factory_.GetWeakPtr());
}

}  // namespace multidevice