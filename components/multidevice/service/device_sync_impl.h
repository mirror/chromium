// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_
#define COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_

#include <memory>

#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/cryptauth_enrollment_manager.h"
#include "components/cryptauth/cryptauth_service.h"
#include "components/multidevice/service/public/interfaces/device_sync.mojom-shared.h"
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

// TODO(hsuregan): Hook up CryptauthclientFactory properly
class CryptAuthClientFactoryImpl : public cryptauth::CryptAuthClientFactory {
 public:
  CryptAuthClientFactoryImpl();
  ~CryptAuthClientFactoryImpl() override;

  std::unique_ptr<cryptauth::CryptAuthClient> CreateInstance() override;
};

// This class syncs metadata about other devices tied to a given Google account.
// It contacts the back-end to enroll the current device
// and sync down new data about other devices.
class DeviceSyncImpl : public device_sync::mojom::DeviceSync,
                       public cryptauth::CryptAuthEnrollmentManager::Observer,
                       public cryptauth::CryptAuthDeviceManager::Observer {
 public:
  // In the long run, we want DeviceSyncImpl to create the enrollment/device
  // managers internally.  In the short-term, we're going to piggyback on the
  // constructor that accepts enrollment/device managers.
  DeviceSyncImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref,
      identity::mojom::IdentityManagerPtr identity_manager_,
      cryptauth::CryptAuthGCMManager* gcm_manager,
      cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory);

  // For testing purposes only.
  DeviceSyncImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref,
      identity::mojom::IdentityManagerPtr identity_manager,
      cryptauth::CryptAuthGCMManager* gcm_manager,
      cryptauth::CryptAuthDeviceManager* device_manager,
      cryptauth::CryptAuthEnrollmentManager* enrollment_manager,
      cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory);

  ~DeviceSyncImpl() override;

  // mojom::DeviceSync:
  void ForceEnrollmentNow() override;
  void ForceSyncNow() override;
  void GetSyncedDevices(GetSyncedDevicesCallback callback) override;
  void SetCapabilityEnabled(
      const std::string& device_id,
      cryptauth::DeviceCapabilityManager::Capability capability,
      bool enabled,
      SetCapabilityEnabledCallback callback) override;
  void FindEligibleDevicesForCapability(
      cryptauth::DeviceCapabilityManager::Capability capability,
      FindEligibleDevicesForCapabilityCallback callback) override;
  void IsCapabilityPromotable(
      const std::string& device_id,
      cryptauth::DeviceCapabilityManager::Capability capability,
      IsCapabilityPromotableCallback callback) override;
  void GetUserPublicKey(GetUserPublicKeyCallback callback) override;
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
    WAITING_FOR_PRIMARY_ACCOUNT,
    WAITING_FOR_KEY_PAIR_GENERATION,
    READY
  };

  void ForceEnrollmentInternal(bool is_initializing);

  void ForceSyncInternal(bool is_initializing);

  device_sync::mojom::ResultCode ConvertToMojomResultCode(
      std::string result_code);
  std::string GetDeviceID(const std::string public_key);

  void OnPrimaryAccountAvailable(const AccountInfo& account_info,
                                 const identity::AccountState& account_state);
  void OnKeyPairGenerated(const std::string& public_key,
                          const std::string& private_key);

  // Callback for SetCapabilityEnabled
  void CapabilityEnabledCallback(SetCapabilityEnabledCallback callback,
                                 const std::string& response);

  // Callbacks for FindEligibleDevicesForCapability
  void SuccessEligibleDevicesForCapabilityCallback(
      FindEligibleDevicesForCapabilityCallback callback,
      const std::vector<cryptauth::ExternalDeviceInfo>& eligible_devices,
      const std::vector<cryptauth::IneligibleDevice>& ineligible_devices);
  void ErrorEligibleDevicesForCapabilityCallback(
      FindEligibleDevicesForCapabilityCallback callback,
      const std::string& error_code);

  // Callbacks for IsCapabilityPromotable
  void SuccessCapabilityPromotableCallback(
      IsCapabilityPromotableCallback callback,
      bool is_promotable);
  void ErrorCapabilityPromotableCallback(
      IsCapabilityPromotableCallback callback,
      const std::string& result_code);

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  identity::mojom::IdentityManagerPtr identity_manager_;
  std::unique_ptr<cryptauth::CryptAuthGCMManager> gcm_manager_;
  std::unique_ptr<cryptauth::CryptAuthEnrollmentManager> enrollment_manager_;
  std::unique_ptr<cryptauth::CryptAuthDeviceManager> device_manager_;
  std::unique_ptr<cryptauth::SecureMessageDelegateFactory>
      secure_message_delegate_factory_;
  std::unique_ptr<cryptauth::RemoteDeviceProvider> remote_device_provider_;

  std::unique_ptr<CryptAuthClientFactoryImpl> crypt_auth_client_factory_;
  std::unique_ptr<cryptauth::DeviceCapabilityManager>
      device_capability_manager_;

  base::Optional<AccountInfo> primary_account_info_;
  identity::AccountState primary_account_state_;
  mojo::InterfacePtrSet<device_sync::mojom::DeviceSyncObserver> observers_;

  State status_;
  std::string public_key_;
  std::string private_key_;

  base::WeakPtrFactory<DeviceSyncImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DeviceSyncImpl);
};

}  // namespace multidevice

#endif  // COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_
