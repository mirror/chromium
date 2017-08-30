// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_
#define COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_

#include <memory>

#include "components/cryptauth/cryptauth_enrollment_manager.h"
#include "components/cryptauth/remote_device_provider.h"
#include "components/multidevice/service/public/interfaces/device_sync.mojom.h"
#include "components/signin/core/browser/account_info.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace multidevice {

// This class syncs metadata about other devices tied to a given Google account.
// It contacts the back-end to enroll the current device
// and sync down new data about other devices.
class DeviceSyncImpl : public device_sync::mojom::DeviceSync,
                       public cryptauth::SecureMessageDelegateFactory,
                       public cryptauth::CryptAuthEnrollmentManager::Observer,
                       public cryptauth::CryptAuthDeviceManager::Observer {
 public:
  explicit DeviceSyncImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref,
      std::unique_ptr<cryptauth::CryptAuthGCMManager> gcm_manager,
      std::unique_ptr<cryptauth::CryptAuthDeviceManager> device_manager,
      std::unique_ptr<cryptauth::CryptAuthEnrollmentManager>
          enrollment_manager);

  ~DeviceSyncImpl() override;

  // mojom::DeviceSync:
  void ForceEnrollmentNow() override;
  void ForceSyncNow() override;
  void GetSyncedDevices(GetSyncedDevicesCallback callback) override;
  void AddObserver(device_sync::mojom::DeviceSyncObserverPtr observer) override;

  // cryptauth::SecureMessageDelegateFactory
  std::unique_ptr<cryptauth::SecureMessageDelegate>
  CreateSecureMessageDelegate() override;

  // cryptauth::CryptAuthEnrollmentManager::Observer:
  void OnEnrollmentStarted() override;
  void OnEnrollmentFinished(bool success) override;

  // cryptauth::CryptAuthDeviceManager::Observer:
  void OnSyncStarted() override;
  void OnSyncFinished(cryptauth::CryptAuthDeviceManager::SyncResult sync_result,
                      cryptauth::CryptAuthDeviceManager::DeviceChangeResult
                          device_change_result) override;

 private:
  void GenerateKeyPair(base::Closure OnKeyPairGenerated);
  void ContructRemoteDeviceProvider(
      base::Closure OnRemoteDeviceProviderConstructed);
  void OnPrimaryAccountAvailable(base::Closure OnAccountInfoStored,
                                 const AccountInfo& account_info,
                                 const identity::AccountState& account_state);
  void OnGenerateKeyPair(base::Closure OnKeyPairStored,
                         const std::string& public_key,
                         const std::string& private_key);

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  std::unique_ptr<cryptauth::CryptAuthGCMManager> gcm_manager_;
  std::unique_ptr<cryptauth::CryptAuthEnrollmentManager> enrollment_manager_;
  std::unique_ptr<cryptauth::CryptAuthDeviceManager> device_manager_;
  std::unique_ptr<cryptauth::RemoteDeviceProvider> remote_device_provider_;

  identity::mojom::IdentityManagerPtr identity_manager_;
  base::Optional<AccountInfo> primary_account_info_;
  identity::AccountState primary_account_state_;
  mojo::InterfacePtrSet<device_sync::mojom::DeviceSyncObserver> observers_;

  std::string public_key_;
  std::string private_key_;
  base::WeakPtrFactory<DeviceSyncImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DeviceSyncImpl);
};

}  // namespace multidevice

#endif  // COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_
