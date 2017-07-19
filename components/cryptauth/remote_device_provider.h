// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_PROVIDER_H_
#define COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_PROVIDER_H_

#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/remote_device_loader.h"
#include "components/cryptauth/secure_message_delegate.h"

namespace cryptauth {

class SecureMessageDelegate;
class SecureMessageDelegateFactory {
 public:
  virtual std::unique_ptr<SecureMessageDelegate>
  CreateSecureMessageDelegate() = 0;
  virtual ~SecureMessageDelegateFactory(){};
};

class RemoteDeviceProvider : public CryptAuthDeviceManager::Observer {
 public:
  RemoteDeviceProvider(
      CryptAuthDeviceManager* device_manager,
      const std::string user_id,
      const std::string user_private_key,
      SecureMessageDelegateFactory* secure_message_delegate_factory);

  ~RemoteDeviceProvider() override;

  class Observer {
   public:
    // Called when a sync attempt is started.
    virtual void OnSyncDeviceListchanged() {}

    virtual ~Observer() {}
  };

  // Adds an observer.
  void AddObserver(Observer* observer);

  // Removes an observer
  void RemoveObserver(Observer* observer);

  // Called when a sync attempt finishes with the |success| of the request.
  // |devices_changed| specifies if the sync caused the stored unlock keys to
  // change.
  void OnSyncFinished(
      CryptAuthDeviceManager::SyncResult sync_result,
      CryptAuthDeviceManager::DeviceChangeResult device_change_result) override;

  // Returns a list of all remote devices that have been synced.
  RemoteDeviceList GetSyncedDevices();

 private:
  // The id of the observer who is observing the remote devices.
  const std::string user_id_;

  // The private key of the observer who is observing the remote devices.
  const std::string user_private_key_;

  // Used to get the remote devices.
  std::unique_ptr<RemoteDeviceLoader> remote_device_loader_;

  // To get ExternalDeviceInfo needed to retrieve remote devices.
  CryptAuthDeviceManager* device_manager_;

  // Contains the remove devices.
  RemoteDeviceList remote_device_list_;

  // Observers observing this observer.
  base::ObserverList<Observer> observers_;

  // Callback used once remote devices are fetched.
  void OnRemoteDevicesLoaded(const RemoteDeviceList& remote_device_list);

  // Needed to generate a secure_message_delegate used by the remote device
  // loader.
  SecureMessageDelegateFactory* secure_message_delegate_factory_;

  // Used to generate a callback.
  base::WeakPtrFactory<RemoteDeviceProvider> weak_ptr_factory_;
};
}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_PROVIDER_H_
