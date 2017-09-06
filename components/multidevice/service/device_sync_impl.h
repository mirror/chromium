// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_
#define COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_

#include <memory>

#include "components/cryptauth/cryptauth_access_token_fetcher.h"
#include "components/cryptauth/cryptauth_client.h"
#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/cryptauth_enroller.h"
#include "components/cryptauth/cryptauth_enrollment_manager.h"
#include "components/cryptauth/cryptauth_service.h"
#include "components/cryptauth/secure_message_delegate.h"
#include "components/multidevice/service/public/interfaces/device_sync.mojom.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/pref_service_factory.h"
#include "components/signin/core/browser/account_info.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/identity/public/interfaces/identity_manager.mojom.h"
#include "services/preferences/public/interfaces/preferences.mojom.h"
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
  // This constructor creates its own CryptAuthEnrollmentManager and
  // CryptAuthDeviceManager instances internally; when the MultiDevice service
  // is complete, this will be the only constructor.
  DeviceSyncImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref,
      scoped_refptr<net::URLRequestContextGetter> request_context,
      identity::mojom::IdentityManagerPtr identity_manager,
      prefs::mojom::PrefStoreConnectorPtr pref_connector,
      cryptauth::CryptAuthGCMManager* gcm_manager,
      cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory);

  // This constructor takes CryptAuthEnrollmentManager and
  // CryptAuthDeviceManager as parameters. This constructor will be used while
  // MultiDevice service is being built, since these manager objects will be
  // shared with ChromeCryptAuthService in this interim state.
  // TODO(khorimotot): Eliminate when MultiDeviceService is done.
  DeviceSyncImpl(
      std::unique_ptr<service_manager::ServiceContextRef> service_ref,
      identity::mojom::IdentityManagerPtr identity_manager,
      prefs::mojom::PrefStoreConnectorPtr pref_connector,
      cryptauth::CryptAuthGCMManager* gcm_manager,
      cryptauth::CryptAuthDeviceManager* device_manager,
      cryptauth::CryptAuthEnrollmentManager* enrollment_manager,
      cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory);

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
    WAITING_FOR_PRIMARY_ACCOUNT,
    WAITING_FOR_PREFS,
    WAITING_FOR_ENROLLMENT_MANAGER_INIT,
    READY
  };

  cryptauth::CryptAuthEnrollmentManager* GetEnrollmentManager();
  cryptauth::CryptAuthDeviceManager* GetDeviceManager();

  void Initialize();
  void ConnectToPrefService();
  void CreateManagers();
  void StartManagers();
  void FinishPostEnrollmentInitialization();
  void InitialEnroll();

  void OnPrimaryAccountAvailable(const AccountInfo& account_info,
                                 const identity::AccountState& account_state);
  void OnConnectedToPrefService(std::unique_ptr<PrefService> pref_service);

  void ForceEnrollmentInternal(bool is_initializing);
  void ForceSyncInternal(bool is_initializing);

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  identity::mojom::IdentityManagerPtr identity_manager_;
  prefs::mojom::PrefStoreConnectorPtr pref_connector_;
  std::unique_ptr<cryptauth::CryptAuthGCMManager> gcm_manager_;
  std::unique_ptr<PrefService> pref_service_;
  // Depending on which constructor is used, either the unique or raw pointers
  // will be defined, but not both. When using these pointers, do not
  // reference them directly; instead, use GetEnrollmentManager() and
  // GetDeviceManager();
  std::unique_ptr<cryptauth::CryptAuthEnrollmentManager> enrollment_manager_;
  std::unique_ptr<cryptauth::CryptAuthDeviceManager> device_manager_;
  cryptauth::CryptAuthEnrollmentManager* enrollment_manager_raw_ = nullptr;
  cryptauth::CryptAuthDeviceManager* device_manager_raw_ = nullptr;

  cryptauth::SecureMessageDelegateFactory* secure_message_delegate_factory_;
  std::unique_ptr<cryptauth::RemoteDeviceProvider> remote_device_provider_;

  AccountInfo primary_account_info_;
  identity::AccountState primary_account_state_;
  mojo::InterfacePtrSet<device_sync::mojom::DeviceSyncObserver> observers_;

  State state_;

  // True when the constructor which takes the device/enrollment
  // managers as parameters is called; false otherwise.
  const bool managers_were_injected_;

  // Null when |managers_were_injected_| is true; non-null otherwise.
  scoped_refptr<net::URLRequestContextGetter> request_context_;

  base::WeakPtrFactory<DeviceSyncImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(DeviceSyncImpl);
};

}  // namespace multidevice

#endif  // COMPONENTS_MULTIDEVICE_DEVICE_SYNC_IMPL_H_
