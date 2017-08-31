// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_
#define COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_

#include <memory>

#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/cryptauth_enrollment_manager.h"
#include "components/cryptauth/cryptauth_service.h"
#include "components/multidevice/service/public/interfaces/device_sync.mojom.h"
#include "components/signin/core/browser/account_info.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/service_manager/public/cpp/service_context_ref.h"

namespace cryptauth {
class RemoteDeviceProvider;
class SecureMessageDelegateFactory;
}  // namespace cryptauth

namespace multidevice {

// This class syncs metadata about other devices tied to a given Google account.
// It contacts the back-end to enroll the current device
// and sync down new data about other devices.
class DeviceSyncImpl : public device_sync::mojom::DeviceSync,
                       public cryptauth::CryptAuthEnrollmentManager::Observer,
                       public cryptauth::CryptAuthDeviceManager::Observer {
 public:
  DeviceSyncImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref,
      service_manager::Connector* connector,
      cryptauth::CryptAuthGCMManager* gcm_manager,
      cryptauth::CryptAuthDeviceManager* device_manager,
      cryptauth::CryptAuthEnrollmentManager* enrollment_manager,
      cryptauth::SecureMessageDelegateFactory* secure_message_delegate);

  ~DeviceSyncImpl() override;

  // mojom::DeviceSync:
  void ForceEnrollmentNow() override;
  void ForceSyncNow() override;
  void GetSyncedDevices(GetSyncedDevicesCallback callback) override;
  void AddObserver(device_sync::mojom::DeviceSyncObserverPtr observer) override;

  // cryptauth::CryptAuthEnrollmentManager::Observer:
  void OnEnrollmentStarted() override;
  void OnEnrollmentFinished(bool success) override;

  // cryptauth::CryptAuthDeviceManager::Observer:
  void OnSyncStarted() override;
  void OnSyncFinished(cryptauth::CryptAuthDeviceManager::SyncResult sync_result,
                      cryptauth::CryptAuthDeviceManager::DeviceChangeResult
                          device_change_result) override;

 private:
  enum class State {
    DISCONNECTED,
    WAITING_FOR_PRIMARY_ACCOUNT,
    WAITING_FOR_KEY_PAIR_GENERATION,
    WAITING_FOR_ACCOUNT_ENROLLMENT,
    WAITING_FOR_DEVICE_SYNC,
    NOTIFY_PRIMARY_ACCOUNT_AVAILABLE,
    NOTIFY_REMOTE_DEVICE_PROVIDER_INITIALIZED,
    NOTIFY_KEY_PAIR_GENERATED,
    NOTIFY_ENROLLMENT_FINISHED,
    NOTIFY_SYNC_FINISHED,
  };

  State GetAndSetState(State new_state);

  void GenerateKeyPair(base::Closure OnKeyPairGenerated);
  void ContructRemoteDeviceProvider(
      base::Closure OnRemoteDeviceProviderConstructed);
  void OnPrimaryAccountAvailable(base::Closure OnAccountInfoStored,
                                 const AccountInfo& account_info,
                                 const identity::AccountState& account_state);
  void OnKeyPairGenerated(base::Closure OnKeyPairStored,
                          const std::string& public_key,
                          const std::string& private_key);

  base::Callback<void(const AccountInfo& account_info,
                      const identity::AccountState& account_state)>
  OnPrimaryAccountAvailableCallback(base::Closure OnAccountInfoStored);

  base::Closure GenerateKeyPairCallback(base::Closure OnKeyPairGenerated);

  base::Closure ConstructRemoteDeviceProviderCallback(
      base::Closure OnRemoteDeviceProviderConstructed);

  base::Closure ForceEnrollmentNowCallback();

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  std::unique_ptr<service_manager::Connector> connector_;
  std::unique_ptr<cryptauth::CryptAuthGCMManager> gcm_manager_;
  std::unique_ptr<cryptauth::CryptAuthEnrollmentManager> enrollment_manager_;
  std::unique_ptr<cryptauth::CryptAuthDeviceManager> device_manager_;
  std::unique_ptr<cryptauth::SecureMessageDelegateFactory>
      secure_message_delegate_factory_;
  State current_state_;

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
