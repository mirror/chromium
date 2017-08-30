// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/device_sync_impl.h"

#include "chrome/browser/chromeos/login/easy_unlock/secure_message_delegate_chromeos.h"
#include "content/public/renderer/render_thread.h"
#include "services/identity/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"

namespace multidevice {

DeviceSyncImpl::DeviceSyncImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref,
    std::unique_ptr<cryptauth::CryptAuthGCMManager> gcm_manager,
    std::unique_ptr<cryptauth::CryptAuthDeviceManager> device_manager,
    std::unique_ptr<cryptauth::CryptAuthEnrollmentManager> enrollment_manager)
    : service_ref_(std::move(service_ref)),
      gcm_manager_(std::move(gcm_manager)),
      enrollment_manager_(std::move(enrollment_manager)),
      device_manager_(std::move(device_manager)),
      weak_ptr_factory_(this) {
  gcm_manager_->StartListening();

  content::RenderThread::Get()->GetConnector()->BindInterface(
      identity::mojom::kServiceName, mojo::MakeRequest(&identity_manager_));

  // RemoteDeviceProvider can only be constructed when the private key is
  // generated.  Private key can only be generated when primary account info is
  // available.  On completing RemoteDeviceProvider construction, call a force
  // enrollment and sync.
  identity_manager_->GetPrimaryAccountWhenAvailable(base::Bind(
      &DeviceSyncImpl::OnPrimaryAccountAvailable,
      weak_ptr_factory_.GetWeakPtr(),
      base::Bind(&DeviceSyncImpl::GenerateKeyPair,
                 weak_ptr_factory_.GetWeakPtr(),
                 base::Bind(&DeviceSyncImpl::ContructRemoteDeviceProvider,
                            weak_ptr_factory_.GetWeakPtr(),
                            base::Bind(&DeviceSyncImpl::ForceEnrollmentNow,
                                       weak_ptr_factory_.GetWeakPtr())))));
}

DeviceSyncImpl::~DeviceSyncImpl() {}

void DeviceSyncImpl::ForceEnrollmentNow() {
  enrollment_manager_->AddObserver(this);
  enrollment_manager_->Start();
}

void DeviceSyncImpl::ForceSyncNow() {
  device_manager_->AddObserver(this);
  device_manager_->Start();
}

void DeviceSyncImpl::GetSyncedDevices(GetSyncedDevicesCallback callback) {
  std::move(callback).Run(remote_device_provider_->GetSyncedDevices());
}

void DeviceSyncImpl::AddObserver(
    device_sync::mojom::DeviceSyncObserverPtr observer) {
  observers_.AddPtr(std::move(observer));
}

std::unique_ptr<cryptauth::SecureMessageDelegate>
DeviceSyncImpl::CreateSecureMessageDelegate() {
  return base::MakeUnique<chromeos::SecureMessageDelegateChromeOS>();
}

void DeviceSyncImpl::OnEnrollmentStarted() {}

void DeviceSyncImpl::OnEnrollmentFinished(bool success) {
  enrollment_manager_->RemoveObserver(this);
  observers_.ForAllPtrs(
      [&success](device_sync::mojom::DeviceSyncObserver* observer) {
        observer->OnEnrollmentFinished(success /* success */);
      });
  if (success) {
    // TODO(hsuregan): Will there ever be a case if Enrollment should happen and
    // DeviceSync shouldn't?
    ForceSyncNow();
  }
}

void DeviceSyncImpl::OnSyncStarted() {}

void DeviceSyncImpl::OnSyncFinished(
    cryptauth::CryptAuthDeviceManager::SyncResult sync_result,
    cryptauth::CryptAuthDeviceManager::DeviceChangeResult
        device_change_result) {
  observers_.ForAllPtrs([&sync_result](
                            device_sync::mojom::DeviceSyncObserver* observer) {
    observer->OnDevicesSynced(
        sync_result ==
        cryptauth::CryptAuthDeviceManager::SyncResult::SUCCESS /* success */);
  });
  device_manager_->RemoveObserver(this);
}

void DeviceSyncImpl::GenerateKeyPair(base::Closure OnKeyPairGenerated) {
  CreateSecureMessageDelegate()->GenerateKeyPair(
      base::Bind(&DeviceSyncImpl::OnGenerateKeyPair,
                 weak_ptr_factory_.GetWeakPtr(), OnKeyPairGenerated));
}

void DeviceSyncImpl::ContructRemoteDeviceProvider(
    base::Closure OnRemoteDeviceProviderConstructed) {
  DCHECK(primary_account_info_->IsValid());
  DCHECK(!private_key_.empty());
  remote_device_provider_ = base::MakeUnique<cryptauth::RemoteDeviceProvider>(
      device_manager_.get(), primary_account_info_->account_id, private_key_,
      this);
  OnRemoteDeviceProviderConstructed.Run();
}

void DeviceSyncImpl::OnPrimaryAccountAvailable(
    base::Closure OnAccountInfoStored,
    const AccountInfo& account_info,
    const identity::AccountState& account_state) {
  DCHECK(account_info.IsValid());
  DCHECK(account_state.is_primary_account);
  DCHECK(account_state.has_refresh_token);
  primary_account_info_ = account_info;
  primary_account_state_ = account_state;
  OnAccountInfoStored.Run();
}

void DeviceSyncImpl::OnGenerateKeyPair(base::Closure OnKeyPairStored,
                                       const std::string& public_key,
                                       const std::string& private_key) {
  public_key_ = public_key;
  private_key_ = private_key;
  OnKeyPairStored.Run();
}

}  // namespace multidevice