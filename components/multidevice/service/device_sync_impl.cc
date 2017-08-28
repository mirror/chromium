// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/multidevice/service/device_sync_impl.h"

#include "base/logging.h"
#include "chrome/browser/chromeos/login/easy_unlock/secure_message_delegate_chromeos.h"
#include "components/cryptauth/remote_device.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/renderer/render_thread.h"
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
  identity_manager_->GetPrimaryAccountWhenAvailable(
      base::Bind(&DeviceSyncImpl::OnPrimaryAccountAvailable,
                 weak_ptr_factory_.GetWeakPtr(),
                 base::Bind(&DeviceSyncImpl::ContructRemoteDeviceProvider,
                            weak_ptr_factory_.GetWeakPtr())));
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

// TODO(hsuregan): Replace this RemoteDevicePtr and BeaconSeedPtrs with a
// StructTrait and typemapping.
// TODO(hsuregan): How to get unlock_key and mobile_hostpot_supported info?
// DeviceCapabilityManager?
void DeviceSyncImpl::GetSyncedDevices(GetSyncedDevicesCallback callback) {
  cryptauth::RemoteDeviceList remote_device_list =
      remote_device_provider_->GetSyncedDevices();

  std::vector<device_sync::mojom::RemoteDevicePtr> remote_device_ptr_list;
  for (const auto& remote_device : remote_device_list) {
    std::vector<device_sync::mojom::BeaconSeedPtr> beacon_seeds = {};
    for (auto beacon_seed : remote_device.beacon_seeds) {
      beacon_seeds.emplace_back(device_sync::mojom::BeaconSeed::New(
          beacon_seed.data(),
          base::TimeTicks::UnixEpoch() + base::TimeDelta::FromMilliseconds(
                                             beacon_seed.start_time_millis()),
          base::TimeTicks::UnixEpoch() + base::TimeDelta::FromMilliseconds(
                                             beacon_seed.end_time_millis())));
    }

    remote_device_ptr_list.emplace_back(device_sync::mojom::RemoteDevice::New(
        remote_device.public_key, remote_device.GetDeviceId(),
        remote_device.user_id, remote_device.name, remote_device.unlock_key,
        remote_device.supports_mobile_hotspot, std::move(beacon_seeds)));
  }
  std::move(callback).Run(std::move(remote_device_ptr_list));
}

void DeviceSyncImpl::AddObserver(
    device_sync::mojom::DeviceSyncObserverPtr observer) {
  observers_.AddPtr(std::move(observer));
}

std::unique_ptr<cryptauth::SecureMessageDelegate>
DeviceSyncImpl::CreateSecureMessageDelegate() {
  return base::MakeUnique<chromeos::SecureMessageDelegateChromeOS>();
}

void DeviceSyncImpl::ContructRemoteDeviceProvider() {
  remote_device_provider_ = base::MakeUnique<cryptauth::RemoteDeviceProvider>(
      device_manager_.get(), primary_account_info_->account_id, private_key_,
      this);
  ForceEnrollmentNow();
}

void DeviceSyncImpl::OnPrimaryAccountAvailable(
    base::Closure callback,
    const AccountInfo& account_info,
    const identity::AccountState& account_state) {
  DCHECK(account_info.IsValid());
  // TODO(hsuregan): Figure out how to handle the case of non primary accounts.
  DCHECK(account_state.is_primary_account);
  DCHECK(account_state.has_refresh_token);
  primary_account_info_ = account_info;
  primary_account_state_ = account_state;
  // TODO(hsuregan): Check whether if user_id parameter RemoteDeviceProvider is
  // same as account_id in AccountInfo.
  CreateSecureMessageDelegate()->GenerateKeyPair(
      base::Bind(&DeviceSyncImpl::OnGenerateKeyPair,
                 weak_ptr_factory_.GetWeakPtr(), callback));
}

void DeviceSyncImpl::OnGenerateKeyPair(base::Closure callback,
                                       const std::string& public_key,
                                       const std::string& private_key) {
  public_key_ = public_key;
  private_key_ = private_key;
  callback.Run();
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

void DeviceSyncImpl::OnSyncStarted(){};

// cryptauth::RemoteDeviceLoader::Observer:
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

}  // namespace multidevice