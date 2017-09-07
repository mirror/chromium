// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/fake_remote_device_provider.h"

#include "base/bind.h"
#include "components/cryptauth/remote_device_loader.h"
#include "components/cryptauth/secure_message_delegate.h"

namespace cryptauth {

FakeRemoteDeviceProvider::FakeRemoteDeviceProvider() {}

FakeRemoteDeviceProvider::~FakeRemoteDeviceProvider() {}

void FakeRemoteDeviceProvider::OnSyncFinished(
    CryptAuthDeviceManager::SyncResult sync_result,
    CryptAuthDeviceManager::DeviceChangeResult device_change_result) {
  if (sync_result == CryptAuthDeviceManager::SyncResult::SUCCESS &&
      device_change_result ==
          CryptAuthDeviceManager::DeviceChangeResult::CHANGED) {
    OnRemoteDevicesLoaded(scheduled_synced_remote_devices_);
  }
}

void FakeRemoteDeviceProvider::OnRemoteDevicesLoaded(
    const RemoteDeviceList& synced_remote_devices) {
  synced_remote_devices_ = synced_remote_devices;

  // Notify observers of change. Note that there is no need to check if
  // |synced_remote_devices_| has changed here because the fetch is only started
  // if the change result passed to OnSyncFinished() is CHANGED.
  RemoteDeviceProvider::NotifyObserversDeviceListChanged();
}

void FakeRemoteDeviceProvider::SetRemoteDeviceList(
    const RemoteDeviceList& synced_remote_devices) {
  scheduled_synced_remote_devices_ = synced_remote_devices;
}

const RemoteDeviceList FakeRemoteDeviceProvider::GetSyncedDevices() const {
  return synced_remote_devices_;
}

}  // namespace cryptauth