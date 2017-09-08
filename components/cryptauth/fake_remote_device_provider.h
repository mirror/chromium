// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_PROVIDER_IMPL_H_
#define COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_PROVIDER_IMPL_H_

#include "components/cryptauth/remote_device_provider.h"

namespace cryptauth {

// This class generates and caches RemoteDevice objects when associated metadata
// has been synced, and updates this cache when a new sync occurs.
class FakeRemoteDeviceProvider : public RemoteDeviceProvider {
 public:
  FakeRemoteDeviceProvider();

  ~FakeRemoteDeviceProvider() override;

  // Returns a list of all RemoteDevices that have been synced.
  const RemoteDeviceList GetSyncedDevices() const override;

  void set_synced_remote_devices(
      const RemoteDeviceList& synced_remote_devices) {
    synced_remote_devices_ = synced_remote_devices;
  }

 private:
  RemoteDeviceList synced_remote_devices_;

  DISALLOW_COPY_AND_ASSIGN(FakeRemoteDeviceProvider);
};

}  // namespace cryptauth

#endif  // COMPONENTS_CRYPTAUTH_REMOTE_DEVICE_PROVIDER_IMPL_H_
