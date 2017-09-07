// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_PROVIDER_IMPL_H_
#define COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_PROVIDER_IMPL_H_

#include "components/cryptauth/cryptauth_device_manager.h"
#include "components/cryptauth/remote_device_provider.h"

namespace cryptauth {

// This class generates and caches RemoteDevice objects when associated metadata
// has been synced, and updates this cache when a new sync occurs.
class FakeRemoteDeviceProvider : public RemoteDeviceProvider,
                                 public CryptAuthDeviceManager::Observer {
 public:
  FakeRemoteDeviceProvider();

  ~FakeRemoteDeviceProvider() override;

  // Returns a list of all RemoteDevices that have been synced.
  const RemoteDeviceList GetSyncedDevices() const override;

  // CryptAuthDeviceManager::Observer:
  void OnSyncFinished(
      CryptAuthDeviceManager::SyncResult sync_result,
      CryptAuthDeviceManager::DeviceChangeResult device_change_result) override;

  void SetRemoteDeviceList(const RemoteDeviceList& synced_remote_devices);

 private:
  void OnRemoteDevicesLoaded(const RemoteDeviceList& synced_remote_devices);

  // The account ID of the current user.
  const std::string user_id_;

  // The private key used to generate RemoteDevices.
  const std::string user_private_key_;

  RemoteDeviceList scheduled_synced_remote_devices_;
  RemoteDeviceList synced_remote_devices_;

  DISALLOW_COPY_AND_ASSIGN(FakeRemoteDeviceProvider);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_PROVIDER_IMPL_H_
