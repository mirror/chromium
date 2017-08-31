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
    cryptauth::CryptAuthEnrollmentManager* enrollment_manager,
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

  status_ = State::WAITING_FOR_PRIMARY_ACCOUNT;
  identity_manager_->GetPrimaryAccountWhenAvailable(
      base::Bind(&DeviceSyncImpl::OnPrimaryAccountAvailable,
                 weak_ptr_factory_.GetWeakPtr()));
}

DeviceSyncImpl::~DeviceSyncImpl() {
  enrollment_manager_->RemoveObserver(this);
  device_manager_->RemoveObserver(this);
}

void DeviceSyncImpl::ForceEnrollmentNow() {
  cryptauth::InvocationReason reason;
  switch (status_) {
    case State::NOTIFY_REMOTE_DEVICE_PROVIDER_INITIALIZED:
      reason = cryptauth::InvocationReason::INVOCATION_REASON_INITIALIZATION;
      // TODO(hsuregan): Or, enrollment_manager_->Start();
      break;
    default:
      reason = cryptauth::InvocationReason::INVOCATION_REASON_UNKNOWN;
      break;
  }
  status_ = State::WAITING_FOR_ACCOUNT_ENROLLMENT;
  enrollment_manager_->ForceEnrollmentNow(reason);
}

void DeviceSyncImpl::ForceSyncNow() {
  cryptauth::InvocationReason reason;
  switch (status_) {
    case State::NOTIFY_ENROLLMENT_FINISHED:
      reason = cryptauth::InvocationReason::INVOCATION_REASON_INITIALIZATION;
      // TODO(hsuregan): Or, device_manager_->Start();
      break;
    default:
      reason = cryptauth::InvocationReason::INVOCATION_REASON_UNKNOWN;
      break;
  }
  status_ = State::WAITING_FOR_DEVICE_SYNC;
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
  DCHECK(status_ == State::WAITING_FOR_ACCOUNT_ENROLLMENT);
  status_ = State::NOTIFY_ENROLLMENT_FINISHED;
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
  DCHECK(status_ == State::WAITING_FOR_DEVICE_SYNC);
  status_ = State::NOTIFY_SYNC_FINISHED;
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

void DeviceSyncImpl::OnPrimaryAccountAvailable(
    const AccountInfo& account_info,
    const identity::AccountState& account_state) {
  DCHECK(status_ == State::WAITING_FOR_PRIMARY_ACCOUNT);
  primary_account_info_ = account_info;
  primary_account_state_ = account_state;

  status_ = State::WAITING_FOR_KEY_PAIR_GENERATION;
  secure_message_delegate_factory_->CreateSecureMessageDelegate()
      ->GenerateKeyPair(base::Bind(&DeviceSyncImpl::OnKeyPairGenerated,
                                   weak_ptr_factory_.GetWeakPtr()));
}

void DeviceSyncImpl::OnKeyPairGenerated(const std::string& public_key,
                                        const std::string& private_key) {
  DCHECK(status_ == State::WAITING_FOR_KEY_PAIR_GENERATION);
  status_ = State::NOTIFY_KEY_PAIR_GENERATED;
  public_key_ = public_key;
  private_key_ = private_key;
  if (primary_account_info_.has_value()) {
    DCHECK(primary_account_info_->IsValid());
    DCHECK(!private_key_.empty());
    remote_device_provider_ = base::MakeUnique<cryptauth::RemoteDeviceProvider>(
        device_manager_.get(), primary_account_info_->account_id, private_key_,
        secure_message_delegate_factory_.get());
    status_ = State::NOTIFY_REMOTE_DEVICE_PROVIDER_INITIALIZED;
    ForceSyncNow();
  }
}

}  // namespace multidevice