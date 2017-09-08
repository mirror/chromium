// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cryptauth/fake_remote_device_provider.h"
#include "base/memory/ptr_util.h"

namespace cryptauth {

std::unique_ptr<RemoteDeviceProvider>
FakeRemoteDeviceProvider::Factory::NewInstance() {
  if (!factory_instance_) {
    factory_instance_ = new Factory();
  }
  return factory_instance_->BuildInstance();
}

void FakeRemoteDeviceProvider::Factory::SetInstanceForTesting(
    Factory* factory) {
  factory_instance_ = factory;
}

std::unique_ptr<RemoteDeviceProvider>
FakeRemoteDeviceProvider::Factory::BuildInstance() {
  return base::WrapUnique(new FakeRemoteDeviceProvider());
}

FakeRemoteDeviceProvider::FakeRemoteDeviceProvider() {}

FakeRemoteDeviceProvider::~FakeRemoteDeviceProvider() {}

const RemoteDeviceList FakeRemoteDeviceProvider::GetSyncedDevices() const {
  return synced_remote_devices_;
}

}  // namespace cryptauth