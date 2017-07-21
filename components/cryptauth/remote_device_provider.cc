// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/remote_device_provider.h"
#include "components/cryptauth/remote_device_loader.h"
#include "components/cryptauth/secure_message_delegate.h"

#include "base/bind.h"

namespace cryptauth {

RemoteDeviceProvider::RemoteDeviceProvider(
    CryptAuthDeviceManager* device_manager,
    const std::string user_id,
    const std::string user_private_key,
    SecureMessageDelegateFactory* secure_message_delegate_factory)
    : device_manager_(device_manager),
      user_id_(user_id),
      user_private_key_(user_private_key),
      secure_message_delegate_factory_(secure_message_delegate_factory),
      weak_ptr_factory_(this) {
  remote_device_loader_ = cryptauth::RemoteDeviceLoader::Factory::NewInstance(
      device_manager->GetSyncedDevices(), user_id, user_private_key,
      secure_message_delegate_factory->CreateSecureMessageDelegate());
  device_manager_->AddObserver(
      (cryptauth::CryptAuthDeviceManager::Observer*)this);
  remote_device_loader_->Load(
      false, base::Bind(&RemoteDeviceProvider::OnRemoteDevicesLoaded,
                        weak_ptr_factory_.GetWeakPtr()));
}

RemoteDeviceProvider::~RemoteDeviceProvider() {
  device_manager_->RemoveObserver(this);
}

void RemoteDeviceProvider::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void RemoteDeviceProvider::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void RemoteDeviceProvider::OnRemoteDevicesLoaded(
    const RemoteDeviceList& remote_device_list) {
  remote_device_list_ = remote_device_list;
  remote_device_loader_.reset();

  // Notify all observers of successful change (callback should fail otherwise).
  for (auto& observer : observers_) {
    observer.OnSyncDeviceListchanged();
  }
}

void RemoteDeviceProvider::OnSyncFinished(
    CryptAuthDeviceManager::SyncResult sync_result,
    CryptAuthDeviceManager::DeviceChangeResult device_change_result) {
  if (device_change_result ==
      CryptAuthDeviceManager::DeviceChangeResult::CHANGED) {
    remote_device_loader_ = cryptauth::RemoteDeviceLoader::Factory::NewInstance(
        device_manager_->GetSyncedDevices(), user_id_, user_private_key_,
        secure_message_delegate_factory_->CreateSecureMessageDelegate());

    remote_device_loader_->Load(
        false /* should_load_beacon_seeds */,
        base::Bind(&RemoteDeviceProvider::OnRemoteDevicesLoaded,
                   weak_ptr_factory_.GetWeakPtr()));
  }
}

RemoteDeviceList RemoteDeviceProvider::GetSyncedDevices() {
  return remote_device_list_;
}

}  // namespace cryptauth