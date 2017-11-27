// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/vr/test/fake_vr_device_provider.h"

namespace device {

FakeVRDeviceProvider::FakeVRDeviceProvider() : VRDeviceProvider() {
  initialized_ = false;
}

FakeVRDeviceProvider::~FakeVRDeviceProvider() {}

void FakeVRDeviceProvider::AddDevice(std::unique_ptr<VRDevice> device) {
  devices_.push_back(std::move(device));
  if (initialized_) {
    add_device_callback_.Run(devices_.back().get());
  }
}

void FakeVRDeviceProvider::Initialize(
    base::Callback<void(VRDevice*)> add_device_callback,
    base::Callback<void(VRDevice*)> remove_device_callback,
    base::OnceClosure initialization_complete) {
  add_device_callback_ = std::move(add_device_callback);
  remove_device_callback_ = std::move(remove_device_callback);

  for (std::unique_ptr<VRDevice>& device : devices_) {
    add_device_callback_.Run(device.get());
  }
  initialized_ = true;
  std::move(initialization_complete).Run();
}

bool FakeVRDeviceProvider::Initialized() {
  return initialized_;
}

}  // namespace device
